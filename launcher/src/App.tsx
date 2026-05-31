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
import { message, open } from "@tauri-apps/plugin-dialog";

interface Game {
  id: string;
  name: string;
  path: string;
}

function App() {
  const [activeTab, setActiveTab] = useState("library");
  const [games, setGames] = useState<Game[]>([]);

  const handleLaunch = async (gameId: string, path: string) => {
    try {
      const result = await invoke("launch_game", { path });
      console.log(result);
    } catch (error) {
      if (error === "FILE_NOT_FOUND") {
        await message(`The file at ${path} does not exist. It will be removed from your library.`, {
          title: "File Not Found",
          kind: "error",
        });
        setGames((prev) => prev.filter((g) => g.id !== gameId));
      } else {
        console.error("Failed to launch game:", error);
      }
    }
  };

  const handleAddGame = async () => {
    try {
      const selected = await open({
        multiple: false,
        filters: [{
          name: 'Executable',
          extensions: ['exe']
        }]
      });

      if (selected && typeof selected === 'string') {
        const path = selected;
        // Simple name extraction from path
        const name = path.split(/[\\/]/).pop()?.replace('.exe', '') || "Unknown Game";
        
        const newGame: Game = {
          id: Math.random().toString(36).substr(2, 9),
          name,
          path,
        };

        setGames(prev => [...prev, newGame]);
      }
    } catch (error) {
      console.error("Failed to add game:", error);
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
                    {games.length > 0 ? "Ready to Smooth" : "Welcome"}
                  </div>
                  <h2 className="text-5xl font-black tracking-tight italic uppercase">
                    {games.length > 0 ? games[0].name : "FrameForge"}
                  </h2>
                  <p className="text-gray-400 max-w-md text-sm leading-relaxed">
                    {games.length > 0 
                      ? "Optimized for cinematic smoothing. Stability remains high throughout the session."
                      : "Start your journey by adding a game to your library and enabling universal frame smoothing."}
                  </p>
                  <div className="flex gap-4 mt-2">
                    {games.length > 0 ? (
                      <button 
                        onClick={() => handleLaunch(games[0].id, games[0].path)}
                        className="flex items-center gap-3 bg-nvidia-green hover:bg-[#86d100] text-black px-8 py-3 rounded-full font-black uppercase text-sm transition-all hover:scale-105 active:scale-95 shadow-[0_5px_15px_rgba(118,185,0,0.3)]"
                      >
                        <Play size={18} fill="black" />
                        Launch Game
                      </button>
                    ) : (
                      <button 
                        onClick={handleAddGame}
                        className="flex items-center gap-3 bg-nvidia-green hover:bg-[#86d100] text-black px-8 py-3 rounded-full font-black uppercase text-sm transition-all hover:scale-105 active:scale-95 shadow-[0_5px_15px_rgba(118,185,0,0.3)]"
                      >
                        <Plus size={18} />
                        Add Your First Game
                      </button>
                    )}
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
                  <button 
                    onClick={handleAddGame}
                    className="text-nvidia-green text-sm font-bold flex items-center gap-1 hover:underline"
                  >
                    <Plus size={16} /> Add Game
                  </button>
                </div>
                
                <div className="grid grid-cols-1 xl:grid-cols-2 gap-4">
                  {games.map((game) => (
                    <div 
                      key={game.id}
                      className="group flex gap-5 bg-nvidia-card border border-nvidia-border hover:border-nvidia-green/30 p-4 rounded-2xl transition-all hover:shadow-[0_8px_30px_rgba(0,0,0,0.5)] cursor-pointer"
                    >
                      <div className="w-20 h-20 bg-[#1a1a1a] rounded-xl flex items-center justify-center shrink-0 group-hover:scale-105 transition-transform">
                        <Monitor size={28} className="text-gray-700" />
                      </div>
                      <div className="flex-1 flex flex-col justify-center gap-1 min-w-0">
                        <h4 className="font-bold text-lg truncate">{game.name}</h4>
                        <div className="flex items-center gap-4 text-xs text-gray-500 font-medium">
                          <div className="flex items-center gap-1">
                            <CheckCircle2 size={12} className="text-nvidia-green" />
                            DX11 Ready
                          </div>
                          <div className="flex items-center gap-1 hover:text-white transition-colors cursor-pointer">
                            <Settings size={12} />
                            Game Settings
                          </div>
                        </div>
                      </div>
                      <div className="flex gap-2 self-center">
                        <button 
                          onClick={() => handleLaunch(game.id, game.path)}
                          className="p-3 rounded-xl bg-nvidia-green/10 hover:bg-nvidia-green text-nvidia-green hover:text-black transition-all"
                          title="Launch"
                        >
                          <Play size={20} fill="currentColor" />
                        </button>
                      </div>
                    </div>
                  ))}
                  {games.length === 0 && (
                    <div className="col-span-full py-20 border-2 border-dashed border-nvidia-border rounded-3xl flex flex-col items-center justify-center text-gray-500 gap-4">
                      <Library size={48} className="opacity-20" />
                      <p className="font-bold tracking-wide">No games found in your library.</p>
                      <button 
                        onClick={handleAddGame}
                        className="bg-nvidia-green/10 text-nvidia-green px-6 py-2 rounded-full border border-nvidia-green/20 text-xs font-black uppercase tracking-widest hover:bg-nvidia-green/20 transition-all"
                      >
                        Add Game Manually
                      </button>
                    </div>
                  )}
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
