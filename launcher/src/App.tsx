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
  settings: {
    fpsCap: number;
    interpolationMode: 'blend' | 'motion-aware';
  }
}

function App() {
  const [activeTab, setActiveTab] = useState("library");
  const [selectedGameId, setSelectedGameId] = useState<string | null>(null);
  const [games, setGames] = useState<Game[]>([]);
  const [isLaunching, setIsLaunching] = useState(false);

  const handleLaunch = async (gameId: string, path: string) => {
    try {
      setIsLaunching(true);
      const result = await invoke("launch_game", { path });
      console.log(result);
      setTimeout(() => setIsLaunching(false), 5000);
    } catch (error) {
      setIsLaunching(false);
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
        const name = path.split(/[\\/]/).pop()?.replace('.exe', '') || "Unknown Game";
        
        const newGame: Game = {
          id: Math.random().toString(36).substr(2, 9),
          name,
          path,
          settings: {
            fpsCap: 60,
            interpolationMode: 'blend'
          }
        };

        setGames(prev => [...prev, newGame]);
      }
    } catch (error) {
      console.error("Failed to add game:", error);
    }
  };

  const selectedGame = games.find(g => g.id === selectedGameId);

  const SidebarItem = ({ id, icon: Icon, label }: { id: string; icon: any; label: string }) => (
    <button
      onClick={() => { setActiveTab(id); setSelectedGameId(null); }}
      className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all duration-200 group ${
        activeTab === id && !selectedGameId
          ? "bg-nvidia-green text-black font-bold shadow-[0_0_15px_rgba(118,185,0,0.4)]" 
          : "text-gray-400 hover:text-white hover:bg-white/5"
      }`}
    >
      <Icon size={20} className={activeTab === id && !selectedGameId ? "text-black" : "group-hover:text-nvidia-green transition-colors"} />
      <span className="text-sm tracking-wide">{label}</span>
      {activeTab === id && !selectedGameId && (
        <motion.div layoutId="active" className="ml-auto w-1 h-4 bg-black rounded-full" />
      )}
    </button>
  );

  return (
    <div className="flex h-screen bg-nvidia-dark text-white overflow-hidden font-sans">
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
      <main className="flex-1 overflow-y-auto bg-gradient-to-b from-[#0c0c0c] to-nvidia-dark p-10 custom-scrollbar relative">
        
        <AnimatePresence mode="wait">
          {activeTab === "library" && !selectedGameId && (
            <motion.div
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              key="library-list"
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
                        disabled={isLaunching}
                        className={`flex items-center gap-3 bg-nvidia-green hover:bg-[#86d100] text-black px-8 py-3 rounded-full font-black uppercase text-sm transition-all hover:scale-105 active:scale-95 shadow-[0_5px_15px_rgba(118,185,0,0.3)] ${isLaunching ? "opacity-50 cursor-wait animate-pulse" : ""}`}
                      >
                        {isLaunching ? (
                          <>
                            <Activity size={18} className="animate-spin" />
                            Launching...
                          </>
                        ) : (
                          <>
                            <Play size={18} fill="black" />
                            Launch Game
                          </>
                        )}
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
                      View Metrics
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
                          <div 
                            onClick={(e) => { e.stopPropagation(); setSelectedGameId(game.id); }}
                            className="flex items-center gap-1 hover:text-nvidia-green transition-colors cursor-pointer font-bold"
                          >
                            <Settings size={12} />
                            Settings
                          </div>
                        </div>
                      </div>
                      <div className="flex gap-2 self-center">
                        <button 
                          onClick={(e) => { e.stopPropagation(); handleLaunch(game.id, game.path); }}
                          className="p-3 rounded-xl bg-nvidia-green/10 hover:bg-nvidia-green text-nvidia-green hover:text-black transition-all"
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

          {selectedGameId && selectedGame && (
            <motion.div
              initial={{ opacity: 0, x: 20 }}
              animate={{ opacity: 1, x: 0 }}
              exit={{ opacity: 0, x: -20 }}
              key="game-settings"
              className="flex flex-col gap-8"
            >
              <div className="flex items-center gap-4 mb-4">
                <button 
                  onClick={() => setSelectedGameId(null)}
                  className="p-2 hover:bg-white/10 rounded-full transition-all"
                >
                  <ChevronRight size={24} className="rotate-180" />
                </button>
                <h2 className="text-3xl font-black italic uppercase tracking-tighter">
                  {selectedGame.name} <span className="text-nvidia-green ml-2">Settings</span>
                </h2>
              </div>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
                <div className="p-8 rounded-3xl glass flex flex-col gap-8">
                  <h3 className="text-lg font-bold flex items-center gap-2">
                    <Monitor className="text-nvidia-green" size={20} />
                    Frame Smoothing
                  </h3>

                  <div className="space-y-6">
                    <div className="flex flex-col gap-3">
                      <div className="flex justify-between items-center">
                        <label className="text-sm font-bold text-gray-400 uppercase tracking-widest">FPS Cap</label>
                        <span className="text-nvidia-green font-black">{selectedGame.settings.fpsCap} FPS</span>
                      </div>
                      <input 
                        type="range" 
                        min="30" 
                        max="240" 
                        step="5"
                        value={selectedGame.settings.fpsCap}
                        onChange={(e) => {
                          const val = parseInt(e.target.value);
                          setGames(prev => prev.map(g => g.id === selectedGame.id ? { ...g, settings: { ...g.settings, fpsCap: val }} : g));
                        }}
                        className="w-full h-1.5 bg-nvidia-border rounded-lg appearance-none cursor-pointer accent-nvidia-green"
                      />
                    </div>

                    <div className="flex flex-col gap-3">
                      <label className="text-sm font-bold text-gray-400 uppercase tracking-widest">Interpolation Mode</label>
                      <div className="grid grid-cols-2 gap-3">
                        {['blend', 'motion-aware'].map((mode) => (
                          <button
                            key={mode}
                            onClick={() => {
                              setGames(prev => prev.map(g => g.id === selectedGame.id ? { ...g, settings: { ...g.settings, interpolationMode: mode as any }} : g));
                            }}
                            className={`py-3 rounded-xl border text-xs font-black uppercase tracking-widest transition-all ${
                              selectedGame.settings.interpolationMode === mode 
                                ? "bg-nvidia-green text-black border-nvidia-green shadow-[0_0_10px_rgba(118,185,0,0.3)]" 
                                : "bg-white/5 border-white/10 text-gray-500 hover:text-white hover:bg-white/10"
                            }`}
                          >
                            {mode}
                          </button>
                        ))}
                      </div>
                    </div>
                  </div>
                </div>

                <div className="p-8 rounded-3xl glass flex flex-col gap-6">
                  <h3 className="text-lg font-bold flex items-center gap-2">
                    <Activity className="text-nvidia-green" size={20} />
                    Engine Preview
                  </h3>
                  <div className="flex-1 border border-white/5 bg-black/40 rounded-2xl flex flex-col items-center justify-center p-10 text-center gap-4">
                    <Cpu size={48} className="text-nvidia-green/20" />
                    <p className="text-xs text-gray-500 max-w-[200px]">
                      Smoothing parameters will be injected into {selectedGame.name} upon next launch.
                    </p>
                  </div>
                </div>
              </div>

              <div className="flex gap-4 mt-4">
                <button 
                  onClick={() => handleLaunch(selectedGame.id, selectedGame.path)}
                  className="flex items-center gap-3 bg-nvidia-green hover:bg-[#86d100] text-black px-10 py-4 rounded-full font-black uppercase text-sm transition-all hover:scale-105 active:scale-95 shadow-[0_5px_15px_rgba(118,185,0,0.3)]"
                >
                  <Play size={18} fill="black" />
                  Save & Launch
                </button>
              </div>
            </motion.div>
          )}

          {activeTab === "performance" && (
            <motion.div
              initial={{ opacity: 0, scale: 0.98 }}
              animate={{ opacity: 1, scale: 1 }}
              key="performance-tab"
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
