use std::ffi::OsStr;
use std::os::windows::ffi::OsStrExt;
use windows::core::PCWSTR;
use windows::Win32::Foundation::{CloseHandle, HANDLE};
use windows::Win32::System::Diagnostics::Debug::WriteProcessMemory;
use windows::Win32::System::LibraryLoader::{GetModuleHandleW, GetProcAddress};
use windows::Win32::System::Memory::{VirtualAllocEx, MEM_COMMIT, MEM_RESERVE, PAGE_READWRITE};
use windows::Win32::System::Threading::{
    CreateProcessW, CreateRemoteThread, ResumeThread, CREATE_SUSPENDED, PROCESS_INFORMATION,
    STARTUPINFOW,
};

pub fn launch_and_inject(exe_path: &str, dll_path: &str) -> Result<(), String> {
    let exe_path_wide: Vec<u16> = OsStr::new(exe_path)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();
    let dll_path_wide: Vec<u16> = OsStr::new(dll_path)
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();

    let mut si = STARTUPINFOW::default();
    let mut pi = PROCESS_INFORMATION::default();

    unsafe {
        let mut exe_path_mutable = exe_path_wide.clone();
        let success = CreateProcessW(
            None,
            Some(windows::core::PWSTR(exe_path_mutable.as_ptr() as *mut _)),
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

        let injection_result = inject_dll(pi.hProcess, &dll_path_wide);
        
        if let Err(e) = injection_result {
            // Clean up if injection fails
            let _ = CloseHandle(pi.hProcess);
            let _ = CloseHandle(pi.hThread);
            return Err(e);
        }

        let _ = ResumeThread(pi.hThread);

        let _ = CloseHandle(pi.hProcess);
        let _ = CloseHandle(pi.hThread);
    }

    Ok(())
}

unsafe fn inject_dll(h_process: HANDLE, dll_path_wide: &[u16]) -> Result<(), String> {
    let dll_path_bytes = dll_path_wide.len() * 2;
    
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

    let mut bytes_written = 0;
    let success = WriteProcessMemory(
        h_process,
        remote_mem,
        dll_path_wide.as_ptr() as *const _,
        dll_path_bytes,
        Some(&mut bytes_written),
    );

    if success.is_err() || bytes_written != dll_path_bytes {
        return Err("Failed to write DLL path to target process memory".to_string());
    }

    let kernel32_name: Vec<u16> = OsStr::new("kernel32.dll")
        .encode_wide()
        .chain(std::iter::once(0))
        .collect();
    
    let h_kernel32 = GetModuleHandleW(PCWSTR(kernel32_name.as_ptr()))
        .map_err(|e| format!("Failed to get kernel32 handle: {}", e))?;

    let load_library_addr = GetProcAddress(h_kernel32, windows::core::s!("LoadLibraryW"));
    
    if load_library_addr.is_none() {
        return Err("Failed to find LoadLibraryW address".to_string());
    }

    let h_thread = CreateRemoteThread(
        h_process,
        None,
        0,
        Some(std::mem::transmute(load_library_addr)),
        Some(remote_mem),
        0,
        None,
    ).map_err(|e| format!("Failed to create remote thread: {}", e))?;

    let _ = CloseHandle(h_thread);

    Ok(())
}
