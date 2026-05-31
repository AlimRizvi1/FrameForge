use std::ffi::{c_void, OsStr};
use std::os::windows::ffi::OsStrExt;

use windows::core::{PCWSTR, PWSTR};
use windows::Win32::Foundation::{CloseHandle, HANDLE, WAIT_FAILED};
use windows::Win32::System::Diagnostics::Debug::WriteProcessMemory;
use windows::Win32::System::LibraryLoader::{GetModuleHandleW, GetProcAddress};
use windows::Win32::System::Memory::{VirtualAllocEx, MEM_COMMIT, MEM_RESERVE, PAGE_READWRITE};
use windows::Win32::System::Threading::{
    CreateProcessW, CreateRemoteThread, GetExitCodeThread, ResumeThread, WaitForSingleObject,
    WaitForInputIdle, CREATE_SUSPENDED, PROCESS_INFORMATION, STARTUPINFOW,
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

    println!("Full DLL path to inject: {}", dll_path);
    println!("Full game executable path: {}", exe_path);

    unsafe {
        // CreateProcessW likes a mutable command line buffer.
        let quoted_path = format!("\"{}\"", exe_path);
        let mut command_line: Vec<u16> = OsStr::new(&quoted_path)
            .encode_wide()
            .chain(std::iter::once(0))
            .collect();

        println!("Creating process: {}", exe_path);

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
            let err = success.err();
            let error_msg = format!(
                "Failed to create process: {:?}\n\nMake sure:\n1. The game path is correct\n2. You have permission to run the executable\n3. The path is quoted when it contains spaces",
                err
            );
            println!("{}", error_msg);
            return Err(error_msg);
        }

        println!("Process created with PID: {}", pi.dwProcessId);
        println!("Injecting DLL: {}", dll_path);

        let remote_thread = match inject_dll(pi.hProcess, &dll_path_wide) {
            Ok(thread) => thread,
            Err(e) => {
                println!("Injection failed: {}", e);
                let _ = CloseHandle(pi.hProcess);
                let _ = CloseHandle(pi.hThread);
                return Err(e);
            }
        };

        println!("DLL injected successfully. Waiting for remote thread...");

        let wait_res = WaitForSingleObject(remote_thread, 5000);
        if wait_res == WAIT_FAILED {
            println!("Warning: WaitForSingleObject failed for remote injection thread.");
        }

        let mut exit_code: u32 = 0;
        if GetExitCodeThread(remote_thread, &mut exit_code).is_ok() {
            println!("Remote thread exit code: {} (0x{:X})", exit_code, exit_code);
            if exit_code == 0 {
                println!("Warning: LoadLibraryW failed in remote process.");
            }
        } else {
            println!("Warning: GetExitCodeThread failed.");
        }

        let _ = CloseHandle(remote_thread);

        // The target process is still suspended (main thread). This pause
        // allows you to attach a native debugger to PID {} before the
        // main thread runs and the injected DLL finishes initialization.
        println!("Resuming process immediately...");
        let resume_result = ResumeThread(pi.hThread);
        if resume_result == u32::MAX {
            println!("Warning: ResumeThread failed.");
        }

        // Give the DLL time to initialize and show console
        println!("Waiting for DLL initialization (3 seconds)...");
        std::thread::sleep(std::time::Duration::from_secs(3));

        let wait_idle = WaitForInputIdle(pi.hProcess, 5000);
        if wait_idle == u32::MAX {
            println!("Warning: WaitForInputIdle failed or timed out.");
        } else {
            println!("Process is idle and should be running.");
        }

        let mut process_exit_code: u32 = 0;
        if windows::Win32::System::Threading::GetExitCodeProcess(pi.hProcess, &mut process_exit_code).is_ok() {
            if process_exit_code == 259 { // STILL_ACTIVE constant
                println!("✓ Process is still running. Game should be active.");
            } else {
                println!("✗ Process exited with code: {} (0x{:X})", process_exit_code, process_exit_code);
            }
        }

        println!("Launch sequence complete. Closing handles...");
        let _ = CloseHandle(pi.hProcess);
        let _ = CloseHandle(pi.hThread);
    }

    Ok(())
}

unsafe fn inject_dll(h_process: HANDLE, dll_path_wide: &[u16]) -> Result<HANDLE, String> {
    let dll_path_bytes = dll_path_wide.len() * 2;
    println!("Allocating {} bytes in target process for DLL path ({} wide chars)...", dll_path_bytes, dll_path_wide.len());

    let remote_mem = VirtualAllocEx(
        h_process,
        None,
        dll_path_bytes,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE,
    );

    if remote_mem.is_null() {
        return Err("Failed to allocate memory in target process".to_string());
    }

    println!("Allocated remote memory at: {:p}", remote_mem);

    let mut bytes_written = 0usize;
    let write_result = WriteProcessMemory(
        h_process,
        remote_mem,
        dll_path_wide.as_ptr() as *const c_void,
        dll_path_bytes,
        Some(&mut bytes_written),
    );

    if write_result.is_err() {
        return Err(format!("WriteProcessMemory failed: {:?}", write_result.err()));
    }

    if bytes_written != dll_path_bytes {
        return Err(format!(
            "WriteProcessMemory wrote {} bytes but expected {}",
            bytes_written, dll_path_bytes
        ));
    }

    println!("Successfully wrote {} bytes to target process at {:p}", bytes_written, remote_mem);

    let kernel32_name: Vec<u16> = OsStr::new("kernel32.dll")
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();

    let h_kernel32 = GetModuleHandleW(PCWSTR(kernel32_name.as_ptr()))
        .map_err(|e| format!("Failed to get kernel32 handle: {}", e))?;

    let load_library_addr = GetProcAddress(h_kernel32, windows::core::s!("LoadLibraryW"))
        .ok_or_else(|| "Failed to find LoadLibraryW address".to_string())?;

    println!("LoadLibraryW address: {:p}", load_library_addr);

    type ThreadStartRoutine = unsafe extern "system" fn(*mut c_void) -> u32;
    let start_routine: Option<ThreadStartRoutine> = Some(std::mem::transmute(load_library_addr));

    println!("Creating remote thread with remote memory pointer: {:p}", remote_mem);

    let h_thread = CreateRemoteThread(
        h_process,
        None,
        0,
        start_routine,
        Some(remote_mem),
        0,
        None,
    )
    .map_err(|e| format!("Failed to create remote thread: {}", e))?;

    Ok(h_thread)
}