import { useState } from "react";
import { Plus, Play, Settings, Monitor, Activity } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";

interface Game {
  id: string;
  name: string;
  path: string;
}

function App() {
  const [games] = useState<Game[]>([
    { id: "1", name: "Tomb Raider", path: "C:/Games/TombRaider/TombRaider.exe" },
  ]);

  const handleLaunch = async (path: string) => {
    try {
      const result = await invoke("launch_game", { path });
      console.log(result);
    } catch (error) {
      console.error("Failed to launch game:", error);
    }
  };

  return (
    <div className="min-h-screen bg-[#0a0a0a] text-white p-8 font-sans">
      <header className="flex justify-between items-center mb-12">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 bg-blue-600 rounded-lg flex items-center justify-center">
            <Monitor size={24} />
          </div>
          <h1 className="text-2xl font-bold tracking-tight">FrameForge</h1>
        </div>
        <div className="flex gap-4">
          <button className="flex items-center gap-2 bg-[#1a1a1a] hover:bg-[#2a2a2a] px-4 py-2 rounded-md transition-colors text-sm border border-[#333]">
            <Settings size={18} />
            Settings
          </button>
          <button className="flex items-center gap-2 bg-blue-600 hover:bg-blue-700 px-4 py-2 rounded-md transition-colors text-sm font-medium">
            <Plus size={18} />
            Add Game
          </button>
        </div>
      </header>

      <main>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {games.map((game) => (
            <div 
              key={game.id}
              className="group bg-[#111] border border-[#222] rounded-xl overflow-hidden hover:border-blue-500/50 transition-all hover:shadow-[0_0_20px_rgba(37,99,235,0.1)]"
            >
              <div className="h-40 bg-[#1a1a1a] flex items-center justify-center">
                <div className="text-blue-500/20 group-hover:text-blue-500/40 transition-colors">
                  <Monitor size={64} />
                </div>
              </div>
              <div className="p-5">
                <h3 className="text-lg font-semibold mb-1">{game.name}</h3>
                <p className="text-xs text-[#666] mb-4 truncate">{game.path}</p>
                <div className="flex justify-between items-center">
                  <div className="flex gap-2">
                    <span className="bg-blue-500/10 text-blue-500 text-[10px] uppercase font-bold px-2 py-0.5 rounded border border-blue-500/20">DX11</span>
                    <span className="bg-green-500/10 text-green-500 text-[10px] uppercase font-bold px-2 py-0.5 rounded border border-green-500/20">Active</span>
                  </div>
                  <button 
                    onClick={() => handleLaunch(game.path)}
                    className="p-2 bg-white text-black rounded-full hover:bg-blue-500 hover:text-white transition-all shadow-lg"
                  >
                    <Play size={20} fill="currentColor" />
                  </button>
                </div>
              </div>
            </div>
          ))}
        </div>

        <div className="mt-12 p-6 bg-[#111] border border-[#222] rounded-xl">
          <div className="flex items-center gap-2 mb-4 text-blue-500">
            <Activity size={20} />
            <h2 className="font-semibold">System Diagnostics</h2>
          </div>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            <div className="bg-[#0a0a0a] p-4 rounded-lg border border-[#222]">
              <p className="text-[#666] text-xs uppercase mb-1">Global State</p>
              <p className="text-xl font-mono text-green-500">Ready</p>
            </div>
            <div className="bg-[#0a0a0a] p-4 rounded-lg border border-[#222]">
              <p className="text-[#666] text-xs uppercase mb-1">Runtime Version</p>
              <p className="text-xl font-mono">v0.1.0-alpha</p>
            </div>
            <div className="bg-[#0a0a0a] p-4 rounded-lg border border-[#222]">
              <p className="text-[#666] text-xs uppercase mb-1">Active Hooks</p>
              <p className="text-xl font-mono">DX11, Win32</p>
            </div>
            <div className="bg-[#0a0a0a] p-4 rounded-lg border border-[#222]">
              <p className="text-[#666] text-xs uppercase mb-1">Process Injected</p>
              <p className="text-xl font-mono text-[#666]">None</p>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}

export default App;
