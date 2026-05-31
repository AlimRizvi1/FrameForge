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
        "F:\\Projects\\FrameForge\\build\\bin\\Release\\FrameForgeRuntime.dll"
    } else {
        "FrameForgeRuntime.dll"
    };

    if !std::path::Path::new(dll_path).exists() {
        return Err(format!("Runtime DLL not found at: {}", dll_path));
    }

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

