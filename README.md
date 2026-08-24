# Dr. Tournaments

**3D Precision Driving Game** — Cross-platform competitive driving with realistic transmission physics, procedural traffic, and multiplayer tournaments.

[![Build](https://github.com/YOUR_USERNAME/DrTournaments/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USERNAME/DrTournaments/actions/workflows/build.yml)

## Features

- 🚗 **Raycast Vehicle Physics** — 4-point suspension, slip-angle tire model, P-R-N-D transmission
- 🛣️ **Procedural Roads** — 1-6 lane highways with dynamic NPC traffic AI
- 🌐 **Cross-Platform Multiplayer** — ENet UDP networking, ghost sync, private servers
- 🎙️ **Voice Chat** — WebRTC (libdatachannel) with Opus codec
- 🏆 **Tournament System** — Competitive racing with anti-cheat input validation
- 📱 **Android + PC** — Same codebase, same servers, full cross-play

## Build (Windows)

```bash
# Prerequisites: Visual Studio 2022, CMake 3.24+, Ninja
cmake --preset windows-release
cmake --build build/windows-release
./build/windows-release/dr_tournaments.exe
```

## Build (Android)

```bash
cd platform/android
./gradlew assembleDebug
```

## Controls (PC)

| Key | Action |
|-----|--------|
| W/S | Gas / Brake |
| A/D | Steer |
| Space | Handbrake |
| 1-4 / P,R,N,D | Gear Selection |
| Q/E | Left/Right Indicator |
| H | Hazard Lights |
| F | Horn |
| C | Camera Cycle |
| B | Rear View |
| V (hold) | Push-To-Talk |
| Tab | Leaderboard |
| Esc | Settings |

## Architecture

- **Engine**: C++20, Raylib (OpenGL 3.3 / GLES3)
- **Networking**: ENet (UDP), libdatachannel (WebRTC)
- **Physics**: Custom raycast vehicle, fixed 60Hz timestep
- **Memory**: Pre-allocated pools, <220 MB peak RAM

## License

MIT — See [LICENSE](LICENSE)
