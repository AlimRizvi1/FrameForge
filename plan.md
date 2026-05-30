# FrameForge MVP Final Plan

Universal Cinematic Frame Smoothing Tool for Low-End PCs

Version: MVP v1
Platform: Windows
Primary API: DirectX 11
Target Hardware: Low-end laptops and mid-range PCs
Primary Goal: Smooth cinematic gameplay on unstable FPS systems

---

# 1. Product Vision

FrameForge is a Windows desktop application that improves perceived smoothness in cinematic single-player games by:

* stabilizing frame pacing,
* capping unstable framerates,
* generating lightweight interpolated frames,
* smoothing camera motion,
* reducing microstutter.

This is NOT:

* a DLSS competitor,
* an AI frame generation engine,
* a competitive gaming tool.

The focus is:

* story-driven games,
* low-end hardware,
* cinematic smoothness,
* minimal GPU overhead.

---

# 2. Final MVP Scope

## MVP Supports

* Windows 10/11
* DirectX 11 games
* x64 games only
* Single-player games
* Borderless fullscreen and windowed mode
* Lightweight interpolation
* Stable FPS caps
* Overlay diagnostics
* Per-game profiles

## MVP Does NOT Support

* DX12
* Vulkan
* OpenGL
* Multiplayer games
* Anti-cheat protected games
* AI interpolation
* TensorRT
* Optical flow neural networks
* HDR
* Linux

---

# 3. Main User Experience

## Basic Flow

1. User opens FrameForge.exe
2. User adds a game EXE
3. User selects:

   * FPS cap
   * interpolation preset
   * overlay options
4. User launches the game through FrameForge
5. Runtime DLL injects automatically
6. Frame smoothing activates
7. Overlay shows:

   * real FPS
   * displayed FPS
   * frame pacing stats

---

# 4. Core Product Strategy

The MVP is NOT trying to:

* create perfect generated frames,
* maximize FPS numbers.

The MVP IS trying to:

* improve visual smoothness,
* reduce inconsistent frametimes,
* make unstable 30–45 FPS feel smoother,
* maintain low hardware overhead.

---

# 5. Technical Architecture

## Architecture Overview

+------------------------------------------------+

| FrameForge.exe                                     |
| -------------------------------------------------- |
| Modern UI Launcher (Tauri + React)                 |
| +----------------------+-------------------------+ |

```
                   |
                   v
```

+------------------------------------------------+

| Rust/Tauri Native Backend                          |               |     |
| -------------------------------------------------- | ------------- | --- |
| Process Launch                                     | DLL Injection | IPC |
| +----------------------+-------------------------+ |               |     |

```
                   |
                   v
```

+------------------------------------------------+

| FrameForgeRuntime.dll                              |                   |        |               |
| -------------------------------------------------- | ----------------- | ------ | ------------- |
| DX11 Hook                                          | Capture           | Pacing | Interpolation |
| Overlay                                            | Motion Estimation |        |               |
| +------------------------------------------------+ |                   |        |               |

---

# 6. Final Tech Stack

## Frontend UI

* Tauri v2
* React
* TypeScript
* TailwindCSS
* shadcn/ui
* Framer Motion
* Zustand

## Native Backend

* Rust
* Tauri commands
* Windows API integration

## Runtime Engine

* C++20
* DirectX 11
* HLSL shaders
* MinHook
* Dear ImGui

## Build System

* CMake

---

# 7. Runtime Pipeline

## Actual Frame Pipeline

1. Game renders frame N
2. Present hook intercepts frame
3. Backbuffer copied
4. Previous frame retrieved
5. Lightweight motion estimation performed
6. Intermediate frame synthesized
7. Frame pacing controller outputs:

   * real frame
   * generated frame
   * real frame
   * generated frame

---

# 8. Interpolation Method

## MVP Method

Motion-aware lightweight interpolation.

## Technique

* block matching
* adaptive blending
* lightweight motion warp

## Avoid

* AI models
* heavy neural interpolation
* CUDA-only solutions

---

# 9. Main MVP Features

## Feature 1 — Modern Desktop App

Modern launcher UI similar to:

* NVIDIA App
* Steam utilities
* MSI Afterburner

## Feature 2 — Game Library

* add game EXE
* save profiles
* quick launch

## Feature 3 — DLL Injection

* automatic runtime injection
* DX11 detection
* safe fallback

## Feature 4 — FPS Cap System

Stable caps:

* 30
* 40
* 45
* custom

## Feature 5 — Lightweight Frame Generation

Generate ONE intermediate frame:

* 30 → 60
* 40 → 80

## Feature 6 — Frame Pacing

Stabilize presentation timing.

## Feature 7 — Overlay

Show:

* render FPS
* displayed FPS
* frame times
* interpolation state
* preset mode

## Feature 8 — Safe Disable

Instantly disable FG without closing game.

---

# 10. Performance Philosophy

Priority order:

1. Stability
2. Compatibility
3. Smooth pacing
4. Low overhead
5. Interpolation quality

NOT:

* maximum FPS
* ultra-realistic AI frames

---

# 11. Recommended Resolution Targets

For your hardware:

## Development Target

* 720p
* 900p

## Recommended Test FPS

* real 30 FPS
* real 40 FPS

## Recommended Output Targets

* 60 displayed FPS
* 80 displayed FPS

---

# 12. Best Test Games

## Good Initial Test Games

* Tomb Raider (2013)
* Life Is Strange
* Outlast
* BioShock Infinite
* Batman Arkham City

## Avoid Initially

* Cyberpunk 2077
* Alan Wake 2
* UE5 heavy games

---

# 13. Final Folder Structure

```text
FrameForge/
├─ launcher/
├─ runtime/
├─ shared/
├─ external/
├─ profiles/
├─ logs/
├─ docs/
├─ CMakeLists.txt
└─ README.md
```

---

# 14. Runtime Modules

## Hook Module

Hooks DX11 Present.

## Capture Module

Copies backbuffer textures.

## Motion Module

Performs lightweight block motion estimation.

## Synthesis Module

Creates interpolated frame.

## Pacing Module

Controls frame timing and scheduling.

## Overlay Module

Renders debug and control UI.

---

# 15. Development Roadmap

## Milestone 1

Build runtime DLL.

Goal:

* DX11 Present hook works.

## Milestone 2

Capture frames.

Goal:

* previous/current frame buffers work.

## Milestone 3

Add overlay.

Goal:

* FPS and frametime visible.

## Milestone 4

Add frame duplication.

Goal:

* pacing system operational.

## Milestone 5

Add blend interpolation.

Goal:

* visible smoothness improvement.

## Milestone 6

Add motion estimation.

Goal:

* reduced ghosting.

## Milestone 7

Build modern launcher UI.

Goal:

* polished desktop app experience.

---

# 16. Definition of Success

The MVP succeeds if:

* games launch reliably,
* frame pacing improves visibly,
* cinematic motion appears smoother,
* GPU overhead remains low,
* the tool works on weak laptops,
* users can safely disable it.

---

# 17. Final Development Strategy

DO NOT start with:

* AI models,
* Vulkan,
* DX12,
* advanced optical flow.

START with:

* DX11 Present hook,
* frame pacing,
* frame capture,
* lightweight interpolation.

The MVP should feel:

* stable,
* lightweight,
* practical,
* useful on real low-end hardware.

---

# 18. Final Requirements To Install

## REQUIRED SOFTWARE

### 1. Visual Studio 2022 Community

Install:
https://visualstudio.microsoft.com/vs/community/

Select workloads:

* Desktop development with C++
* Game development with C++

Required components:

* MSVC v143
* Windows 10 SDK
* Windows 11 SDK
* CMake tools

---

### 2. Rust

Install:
https://rustup.rs/

Verify:

```bash
rustc --version
cargo --version
```

---

### 3. Node.js LTS

Install:
https://nodejs.org/

Recommended:

* Node 22 LTS

Verify:

```bash
node -v
npm -v
```

---

### 4. Git

Install:
https://git-scm.com/download/win

Verify:

```bash
git --version
```

---

### 5. Tauri CLI

Install:

```bash
cargo install tauri-cli
```

Verify:

```bash
cargo tauri -V
```

---

### 6. CMake

Install:
https://cmake.org/download/

Verify:

```bash
cmake --version
```

---

### 7. Visual Studio Code

Install:
https://code.visualstudio.com/

Recommended extensions:

* C/C++
* rust-analyzer
* Tailwind CSS IntelliSense
* Tauri
* Error Lens

---

### 8. RenderDoc

Install:
https://renderdoc.org/

Used for:

* graphics debugging
* texture inspection
* frame analysis

---

### 9. PIX for Windows

Install:
https://devblogs.microsoft.com/pix/download/

Used for:

* DX11 profiling
* GPU timing
* frame pacing analysis

---

# 19. Final Recommended Development Order

1. Install all tools
2. Create runtime DLL project
3. Hook DX11 Present
4. Add frame capture
5. Add overlay
6. Add pacing
7. Add interpolation
8. Build launcher UI
9. Add profiles/settings
10. Polish and optimize

---

# 20. Final Product Identity

FrameForge is:

“A lightweight cinematic frame smoothing layer for low-end PCs.”

NOT:

* a DLSS clone,
* an AI upscaler,
* a competitive gaming tool.

The focus is:

* smooth cinematic motion,
* stable frametimes,
* weak hardware compatibility,
* practical real-world usage.
