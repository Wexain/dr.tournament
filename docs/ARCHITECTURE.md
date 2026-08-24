# Dr. Tournaments — Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CLIENT APPLICATION                            │
│  ┌─────────────┐ ┌──────────┐ ┌────────────┐ ┌──────────────────┐  │
│  │  Engine Core │ │  Physics │ │  Renderer  │ │   Networking     │  │
│  │  ─ State FSM │ │  ─ Vehicle│ │  ─ Cars    │ │  ─ ENet Client   │  │
│  │  ─ 60Hz Loop │ │  ─ Tires  │ │  ─ HUD     │ │  ─ Voice Chat    │  │
│  │  ─ Pools    │ │  ─ Trans  │ │  ─ PostFX  │ │  ─ State Sync    │  │
│  └─────────────┘ └──────────┘ └────────────┘ └──────────────────┘  │
│  ┌─────────────┐ ┌──────────┐ ┌────────────┐ ┌──────────────────┐  │
│  │   World     │ │  Input   │ │  Social    │ │   Tournament     │  │
│  │  ─ Road Gen │ │  ─ KB/GP │ │  ─ Auth    │ │  ─ Rules         │  │
│  │  ─ Traffic  │ │  ─ Touch │ │  ─ Friends │ │  ─ Moderator     │  │
│  │  ─ Environ  │ │  ─ Remap │ │  ─ Economy │ │  ─ Benchmark     │  │
│  └─────────────┘ └──────────┘ └────────────┘ └──────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                              │ ENet UDP │
                              └────┬─────┘
┌─────────────────────────────────┴───────────────────────────────────┐
│                     DEDICATED SERVER (Headless)                      │
│  ┌──────────────┐ ┌──────────────┐ ┌────────────────────────────┐  │
│  │ Room Manager │ │ Anti-Cheat   │ │ State Relay & Broadcast    │  │
│  │ ─ Create/Join│ │ ─ Input Log  │ │ ─ 30Hz state forwarding    │  │
│  │ ─ Auto-close │ │ ─ Validation │ │ ─ Traffic seed sync        │  │
│  │ ─ Kick/Ban  │ │ ─ Frame Check│ │ ─ Chat relay               │  │
│  └──────────────┘ └──────────────┘ └────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## Physics Pipeline (per frame at 60 Hz)

1. **Input Processing** → Keyboard/Gamepad/Touch → InputState
2. **Suspension Raycast** → 4 rays downward → contact points + normal forces
3. **Spring-Damper Forces** → F = k(L₀ - L) - c·v_susp
4. **Tire Forces** → Slip angle α = atan2(v_lat, |v_fwd|) → friction circle clamp
5. **Transmission** → P/R/N/D state → torque output
6. **Drag + Rolling Resistance** → Aerodynamic model
7. **Integration** → Semi-implicit Euler → position, velocity, quaternion rotation
8. **Collision** → OBB-vs-OBB SAT → impulse resolution

## Memory Architecture

- **Zero heap allocation in game loop** — all objects use FixedPool<T, N>
- Vehicle pool: 32 slots × ~2 KB = 64 KB
- NPC pool: 64 slots × ~2 KB = 128 KB  
- Particle pool: 512 slots × 64 B = 32 KB
- Road chunks: 128 × ~4 KB = 512 KB
- **Peak RAM target: < 220 MB** (including GPU resources)

## Network Architecture

- **ENet UDP** with 3 channels: Reliable, State (unreliable), Voice Signaling
- **30 Hz state sync**: 48-byte StatePacket per player per tick
- **Anti-cheat**: 3-byte CompactInputFrame per tick → ~10.8 KB/min upload
- **Ghost rendering**: Opponents rendered as translucent models, no physics collision
- **Deterministic traffic**: Server broadcasts random seed, clients simulate identical NPC AI

## Build System

- **CMake 3.24+** with FetchContent for all dependencies
- **Targets**: `dr_tournaments` (client), `dr_tournaments_server` (headless)
- **CI/CD**: GitHub Actions → Windows .zip + Android .apk on tag push
