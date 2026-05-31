// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
mod injector;

#[tauri::command]
fn launch_game(path: String) -> Result<String, String> {
    println!("Launching game from path: {}", path);
    
    if !std::path::Path::new(&path).exists() {
        return Err("FILE_NOT_FOUND".to_string());
    }

    // Determine DLL path
    let dll_path = if cfg!(debug_assertions) {
        "F:\\Projects\\FrameForge\\runtime\\build\\bin\\Release\\FrameForgeRuntime.dll"
    } else {
        "FrameForgeRuntime.dll"
    };

    println!("Looking for runtime DLL at: {}", dll_path);

    if !std::path::Path::new(dll_path).exists() {
        let error_msg = format!(
            "Runtime DLL not found at: {}\n\n\
            Please build the runtime first:\n\
            1. Open Visual Studio 2022 or later\n\
            2. Run: cmake -S runtime -B runtime/build -G \"Visual Studio 17 2022\" -A x64\n\
            3. Run: cmake --build runtime/build --config Release\n\n\
            This will generate the DLL at the expected location.",
            dll_path
        );
        println!("{}", error_msg);
        return Err(error_msg);
    }

    println!("Runtime DLL found. Injecting...");
    injector::launch_and_inject(&path, dll_path)?;
    
    Ok(format!("Game launched and injected successfully!"))
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_dialog::init())
        .invoke_handler(tauri::generate_handler![launch_game])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

