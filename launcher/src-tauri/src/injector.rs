use std::ffi::{c_void, OsStr};
use std::os::windows::ffi::OsStrExt;
use std::time::{Duration, Instant};
use std::collections::HashSet;

use windows::core::{PCWSTR, PWSTR};
use windows::Win32::Foundation::{CloseHandle, HANDLE};
use windows::Win32::System::Diagnostics::Debug::WriteProcessMemory;
use windows::Win32::System::LibraryLoader::{GetModuleHandleW, GetProcAddress};
use windows::Win32::System::Memory::{VirtualAllocEx, MEM_COMMIT, MEM_RESERVE, PAGE_READWRITE};
use windows::Win32::System::Diagnostics::ToolHelp::{
    CreateToolhelp32Snapshot, Process32FirstW, Process32NextW, PROCESSENTRY32W, TH32CS_SNAPPROCESS,
};
use windows::Win32::System::Threading::{
    CreateProcessW, CreateRemoteThread, GetExitCodeProcess, OpenProcess, ResumeThread, WaitForSingleObject,
    CREATE_SUSPENDED, PROCESS_ALL_ACCESS, PROCESS_INFORMATION, STARTUPINFOW,
};

pub fn launch_and_inject(exe_path: &str, dll_path: &str) -> Result<(), String> {
    let dll_path_wide: Vec<u16> = OsStr::new(dll_path)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();

    let exe_path_wide: Vec<u16> = OsStr::new(exe_path)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();

    let mut si = STARTUPINFOW::default();
    si.cb = std::mem::size_of::<STARTUPINFOW>() as u32;
    let mut pi = PROCESS_INFORMATION::default();

    println!("[FrameForge] Launching primary: {}", exe_path);

    unsafe {
        let quoted_path = format!("\"{}\"", exe_path);
        let mut command_line: Vec<u16> = OsStr::new(&quoted_path)
            .encode_wide()
            .chain(std::iter::once(0))
            .collect();

        let success = CreateProcessW(
            PCWSTR(exe_path_wide.as_ptr()),
            Some(PWSTR(command_line.as_mut_ptr())),
            None,
            None,
            false,
            CREATE_SUSPENDED,
            None,
            None,
            &si,
            &mut pi,
        );

        if success.is_err() {
            return Err(format!("Failed to create process: {:?}", success.err()));
        }

        let root_pid = pi.dwProcessId;
        let mut tracked_pids = HashSet::new();
        tracked_pids.insert(root_pid);

        println!("[FrameForge] Root process started (PID: {}). Injecting...", root_pid);
        let _ = inject_into_handle(pi.hProcess, &dll_path_wide);

        ResumeThread(pi.hThread);
        let _ = CloseHandle(pi.hProcess);
        let _ = CloseHandle(pi.hThread);

        // Recursive Process Tree Monitor
        println!("[FrameForge] Monitoring process tree for descendants (20s window)...");
        let start_time = Instant::now();
        
        while start_time.elapsed() < Duration::from_secs(20) {
            std::thread::sleep(Duration::from_millis(1000));

            let current_snapshot = get_all_pids_and_parents();
            let mut new_children = Vec::new();

            // Find all descendants of any currently tracked PID
            for (pid, ppid) in &current_snapshot {
                if tracked_pids.contains(ppid) && !tracked_pids.contains(pid) {
                    new_children.push(*pid);
                }
            }

            for child_pid in new_children {
                if is_helper_process(child_pid) {
                    println!("[FrameForge] Skipping helper process: {}", child_pid);
                    tracked_pids.insert(child_pid); // Track it but don't inject
                    continue;
                }

                println!("[FrameForge] Descendant detected (PID: {}). Injecting...", child_pid);
                if let Ok(h_child) = OpenProcess(PROCESS_ALL_ACCESS, false, child_pid) {
                    if inject_into_handle(h_child, &dll_path_wide).is_ok() {
                        println!("[FrameForge] Successfully injected into descendant.");
                    }
                    let _ = CloseHandle(h_child);
                }
                tracked_pids.insert(child_pid);
            }

            // Check if any tracked process is still alive
            let mut any_alive = false;
            for pid in &tracked_pids {
                if let Ok(h) = OpenProcess(PROCESS_ALL_ACCESS, false, *pid) {
                    let mut exit_code = 0u32;
                    if GetExitCodeProcess(h, &mut exit_code).is_ok() && exit_code == 259 {
                        any_alive = true;
                        let _ = CloseHandle(h);
                        break;
                    }
                    let _ = CloseHandle(h);
                }
            }

            if !any_alive {
                println!("[FrameForge] All tracked processes exited. Stopping monitor.");
                break;
            }

            // Optimization: If we have injected into a child and it's been stable for 5 seconds, 
            // we can likely stop the heavy monitoring to save CPU.
            if injected_pids.len() > 1 && start_time.elapsed() > Duration::from_secs(12) {
                println!("[FrameForge] Stabilization reached. Closing monitor.");
                break;
            }
        }
    }

    Ok(())
}

fn is_helper_process(pid: u32) -> bool {
    use windows::Win32::System::Diagnostics::ToolHelp::{Module32FirstW, MODULEENTRY32W, TH32CS_SNAPMODULE, TH32CS_SNAPMODULE32};
    
    // Check process name
    let mut name = String::new();
    unsafe {
        let snapshot = match CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) {
            Ok(h) => h,
            Err(_) => return false,
        };
        let mut entry = PROCESSENTRY32W::default();
        entry.dwSize = std::mem::size_of::<PROCESSENTRY32W>() as u32;
        if Process32FirstW(snapshot, &mut entry).is_ok() {
            while Process32NextW(snapshot, &mut entry).is_ok() {
                if entry.th32ProcessID == pid {
                    name = String::from_utf16_lossy(&entry.szExeFile);
                    name = name.trim_matches(char::from(0)).to_lowercase();
                    break;
                }
            }
        }
        let _ = CloseHandle(snapshot);
    }

    let blacklist = ["crash", "report", "cef", "helper", "service", "overlay", "steam", "epic", "battle", "unity", "unrealcef", "socialclub"];
    blacklist.iter().any(|&s| name.contains(s))
}

unsafe fn inject_into_handle(h_process: HANDLE, dll_path_wide: &[u16]) -> Result<(), String> {
    let dll_path_bytes = dll_path_wide.len() * 2;
    
    let remote_mem = VirtualAllocEx(
        h_process,
        None,
        dll_path_bytes,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE,
    );

    if remote_mem.is_null() {
        return Err("Memory allocation failed".into());
    }

    let mut bytes_written = 0usize;
    let _ = WriteProcessMemory(
        h_process,
        remote_mem,
        dll_path_wide.as_ptr() as *const c_void,
        dll_path_bytes,
        Some(&mut bytes_written),
    );

    let kernel32 = GetModuleHandleW(windows::core::w!("kernel32.dll"))
        .map_err(|e| format!("kernel32 not found: {}", e))?;
    let load_library = GetProcAddress(kernel32, windows::core::s!("LoadLibraryW"))
        .ok_or("LoadLibraryW not found")?;

    let h_thread = CreateRemoteThread(
        h_process,
        None,
        0,
        Some(std::mem::transmute(load_library)),
        Some(remote_mem),
        0,
        None,
    ).map_err(|e| format!("Remote thread failed: {}", e))?;

    WaitForSingleObject(h_thread, 2000);
    let _ = CloseHandle(h_thread);
    Ok(())
}

fn get_all_pids_and_parents() -> Vec<(u32, u32)> {
    let mut pairs = Vec::new();
    unsafe {
        let snapshot = match CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) {
            Ok(h) => h,
            Err(_) => return pairs,
        };

        let mut entry = PROCESSENTRY32W::default();
        entry.dwSize = std::mem::size_of::<PROCESSENTRY32W>() as u32;

        if Process32FirstW(snapshot, &mut entry).is_ok() {
            while Process32NextW(snapshot, &mut entry).is_ok() {
                pairs.push((entry.th32ProcessID, entry.th32ParentProcessID));
            }
        }
        let _ = CloseHandle(snapshot);
    }
    pairs
}
