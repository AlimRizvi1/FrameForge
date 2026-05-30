import { useState } from "react";
import { 
  Play, 
  Settings, 
  LayoutDashboard, 
  Library, 
  Activity, 
  Plus, 
  Monitor, 
  Cpu,
  CheckCircle2,
  AlertCircle,
  ChevronRight
} from "lucide-react";
import { invoke } from "@tauri-apps/api/core";
import { motion, AnimatePresence } from "framer-motion";

interface Game {
  id: string;
  name: string;
  path: string;
  lastPlayed: string;
  fps: number;
}

function App() {
  const [activeTab, setActiveTab] = useState("library");
  const [games] = useState<Game[]>([
    { id: "1", name: "Tomb Raider", path: "C:/Games/TombRaider/TombRaider.exe", lastPlayed: "2 hours ago", fps: 60 },
    { id: "2", name: "BioShock Infinite", path: "C:/Games/BioShock/BioShock.exe", lastPlayed: "Yesterday", fps: 45 },
    { id: "3", name: "Batman Arkham City", path: "C:/Games/Batman/Batman.exe", lastPlayed: "3 days ago", fps: 30 },
  ]);

  const handleLaunch = async (path: string) => {
    try {
      const result = await invoke("launch_game", { path });
      console.log(result);
    } catch (error) {
      console.error("Failed to launch game:", error);
    }
  };

  const SidebarItem = ({ id, icon: Icon, label }: { id: string; icon: any; label: string }) => (
    <button
      onClick={() => setActiveTab(id)}
      className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 group ${
        activeTab === id 
          ? "bg-nvidia-green text-black font-bold shadow-[0_0_15px_rgba(118,185,0,0.4)]" 
          : "text-gray-400 hover:text-white hover:bg-white/5"
      }`}
    >
      <Icon size={20} className={activeTab === id ? "text-black" : "group-hover:text-nvidia-green transition-colors"} />
      <span className="text-sm tracking-wide">{label}</span>
      {activeTab === id && (
        <motion.div layoutId="active" className="ml-auto w-1 h-4 bg-black rounded-full" />
      )}
    </button>
  );

  return (
    <div className="flex h-screen bg-nvidia-dark text-white overflow-hidden">
      {/* Sidebar */}
      <aside className="w-64 border-r border-nvidia-border flex flex-col p-6 gap-8 bg-[#080808]">
        <div className="flex items-center gap-3 px-2">
          <div className="w-8 h-8 bg-nvidia-green rounded flex items-center justify-center shadow-[0_0_10px_rgba(118,185,0,0.5)]">
            <Cpu size={20} className="text-black" />
          </div>
          <h1 className="text-lg font-black tracking-tighter uppercase italic">FrameForge</h1>
        </div>

        <nav className="flex flex-col gap-2">
          <SidebarItem id="dashboard" icon={LayoutDashboard} label="Dashboard" />
          <SidebarItem id="library" icon={Library} label="Game Library" />
          <SidebarItem id="performance" icon={Activity} label="Performance" />
          <SidebarItem id="settings" icon={Settings} label="Global Settings" />
        </nav>

        <div className="mt-auto">
          <div className="p-4 rounded-xl glass flex flex-col gap-3">
            <div className="flex items-center gap-2 text-xs font-bold text-nvidia-green uppercase tracking-widest">
              <div className="w-1.5 h-1.5 rounded-full bg-nvidia-green animate-pulse" />
              Runtime Active
            </div>
            <p className="text-[10px] text-gray-500 leading-relaxed">
              DX11 Hooking Engine v0.1.0-alpha is ready for injection.
            </p>
          </div>
        </div>
      </aside>

      {/* Main Content */}
      <main className="flex-1 overflow-y-auto bg-gradient-to-b from-[#0c0c0c] to-nvidia-dark p-10 custom-scrollbar">
        
        <AnimatePresence mode="wait">
          {activeTab === "library" && (
            <motion.div
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              key="library"
              className="flex flex-col gap-10"
            >
              {/* Hero Section */}
              <section className="relative h-72 rounded-3xl overflow-hidden group">
                <div className="absolute inset-0 bg-gradient-to-r from-black via-black/60 to-transparent z-10" />
                <div className="absolute inset-0 bg-[#1a1a1a] flex items-center justify-center">
                  <Monitor size={120} className="text-white/5" />
                </div>
                
                <div className="relative z-20 h-full p-10 flex flex-col justify-center gap-4">
                  <div className="flex items-center gap-2 bg-nvidia-green/10 text-nvidia-green text-[10px] font-black uppercase tracking-[0.2em] px-3 py-1 rounded-full border border-nvidia-green/20 w-fit">
                    Recently Played
                  </div>
                  <h2 className="text-5xl font-black tracking-tight italic uppercase">Tomb Raider</h2>
                  <p className="text-gray-400 max-w-md text-sm leading-relaxed">
                    Optimized for 60 FPS cinematic smoothing. Stability remains 98% throughout the session.
                  </p>
                  <div className="flex gap-4 mt-2">
                    <button 
                      onClick={() => handleLaunch(games[0].path)}
                      className="flex items-center gap-3 bg-nvidia-green hover:bg-[#86d100] text-black px-8 py-3 rounded-full font-black uppercase text-sm transition-all hover:scale-105 active:scale-95 shadow-[0_5px_15px_rgba(118,185,0,0.3)]"
                    >
                      <Play size={18} fill="black" />
                      Launch Game
                    </button>
                    <button className="flex items-center gap-2 bg-white/10 hover:bg-white/20 px-6 py-3 rounded-full font-bold text-sm backdrop-blur-md transition-all border border-white/10">
                      Settings
                    </button>
                  </div>
                </div>
              </section>

              {/* Game Grid */}
              <section>
                <div className="flex justify-between items-end mb-6">
                  <h3 className="text-xl font-bold tracking-tight">Your Library</h3>
                  <button className="text-nvidia-green text-sm font-bold flex items-center gap-1 hover:underline">
                    <Plus size={16} /> Add Game
                  </button>
                </div>
                
                <div className="grid grid-cols-1 xl:grid-cols-2 gap-4">
                  {games.map((game) => (
                    <div 
                      key={game.id}
                      className="group flex gap-5 bg-nvidia-card border border-nvidia-border hover:border-nvidia-green/30 p-4 rounded-2xl transition-all hover:shadow-[0_8px_30px_rgba(0,0,0,0.5)] cursor-pointer"
                    >
                      <div className="w-24 h-24 bg-[#1a1a1a] rounded-xl flex items-center justify-center shrink-0 group-hover:scale-105 transition-transform">
                        <Monitor size={32} className="text-gray-700" />
                      </div>
                      <div className="flex-1 flex flex-col justify-center gap-1 min-w-0">
                        <div className="flex items-center justify-between">
                          <h4 className="font-bold text-lg truncate">{game.name}</h4>
                          <span className="text-[10px] text-gray-500 font-mono uppercase">{game.lastPlayed}</span>
                        </div>
                        <div className="flex items-center gap-4 text-xs text-gray-500 font-medium">
                          <div className="flex items-center gap-1">
                            <Activity size={12} className="text-nvidia-green" />
                            Target: {game.fps} FPS
                          </div>
                          <div className="flex items-center gap-1">
                            <CheckCircle2 size={12} className="text-nvidia-green" />
                            DX11 Ready
                          </div>
                        </div>
                      </div>
                      <button 
                        onClick={() => handleLaunch(game.path)}
                        className="self-center p-3 rounded-xl bg-white/5 hover:bg-nvidia-green text-white hover:text-black transition-all"
                      >
                        <ChevronRight size={20} />
                      </button>
                    </div>
                  ))}
                </div>
              </section>
            </motion.div>
          )}

          {activeTab === "performance" && (
            <motion.div
              initial={{ opacity: 0, scale: 0.98 }}
              animate={{ opacity: 1, scale: 1 }}
              key="performance"
              className="grid grid-cols-1 md:grid-cols-2 gap-6"
            >
              <div className="p-8 rounded-3xl glass flex flex-col gap-6">
                <div className="flex items-center justify-between">
                  <h3 className="text-lg font-bold flex items-center gap-2">
                    <Activity className="text-nvidia-green" size={20} />
                    Injection Monitor
                  </h3>
                  <div className="bg-nvidia-green/20 text-nvidia-green text-[10px] font-bold px-2 py-1 rounded">HEALTHY</div>
                </div>
                
                <div className="space-y-4">
                  {[
                    { label: "DirectX 11 Hooks", status: "Active" },
                    { label: "Memory Capture", status: "Active" },
                    { label: "Frame Pacing Engine", status: "Active" },
                    { label: "Overlay UI Layer", status: "Standby" },
                  ].map((item) => (
                    <div key={item.label} className="flex justify-between items-center p-4 bg-white/5 rounded-xl border border-white/5">
                      <span className="text-sm text-gray-300">{item.label}</span>
                      <span className={`text-xs font-black uppercase ${item.status === 'Active' ? 'text-nvidia-green' : 'text-gray-500'}`}>
                        {item.status}
                      </span>
                    </div>
                  ))}
                </div>
              </div>

              <div className="p-8 rounded-3xl glass flex flex-col gap-6">
                <h3 className="text-lg font-bold flex items-center gap-2">
                  <AlertCircle className="text-nvidia-green" size={20} />
                  System Resources
                </h3>
                <div className="grid grid-cols-2 gap-4">
                  <div className="p-4 bg-white/5 rounded-xl border border-white/5">
                    <p className="text-[10px] text-gray-500 uppercase font-black mb-1">CPU Overhead</p>
                    <p className="text-2xl font-black italic">0.2%</p>
                  </div>
                  <div className="p-4 bg-white/5 rounded-xl border border-white/5">
                    <p className="text-[10px] text-gray-500 uppercase font-black mb-1">VRAM Reserved</p>
                    <p className="text-2xl font-black italic">128MB</p>
                  </div>
                  <div className="p-4 bg-white/5 rounded-xl border border-white/5">
                    <p className="text-[10px] text-gray-500 uppercase font-black mb-1">Interpolation Latency</p>
                    <p className="text-2xl font-black italic text-nvidia-green">~1.2ms</p>
                  </div>
                  <div className="p-4 bg-white/5 rounded-xl border border-white/5">
                    <p className="text-[10px] text-gray-500 uppercase font-black mb-1">Capture Buffer</p>
                    <p className="text-2xl font-black italic">Double</p>
                  </div>
                </div>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </main>
    </div>
  );
}

export default App;
