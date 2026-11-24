# Architektura — AI Living Maze

## Přehled

AI Living Maze odděluje deterministický 3D herní engine od asynchronního generování GLSL shaderů pomocí AI. Renderovací smyčka nikdy nečeká na síť ani na generování modelu.

```
┌──────────────────────────────────────────────────────────────────────┐
│                         MazeApplication                              │
│  SDL3 Window · Input · Main Loop (60+ FPS, nezávislá na AI)          │
└───────┬──────────────────────────┬─────────────────────────┬─────────┘
        │                          │                         │
        ▼                          ▼                         ▼
┌───────────────┐        ┌───────────────────┐     ┌───────────────────┐
│  World Layer  │        │  AI Shader Engine │     │  Rendering Layer  │
│  MazeGenerator│        │  AIShaderService  │     │  Renderer (GL3.3) │
│  ChunkManager │        │  IAIProvider      │     │  Camera + Frustum │
│  SectorManager│        │   ├ MockAI        │     │  DebugOverlay HUD │
│  World        │        │   └ OpenAI-compat │     └─────────┬─────────┘
└───────┬───────┘        │  ShaderCache      │               │
        │                └─────────┬─────────┘               │
        ▼                          │ async queue             ▼
┌───────────────┐                  ▼                ┌──────────────────┐
│  Game Layer   │        ┌───────────────────┐      │  OpenGL 3.3 GPU  │
│  Player       │        │  ShaderManager    │─────▶│  VAO/VBO / chunk │
│  CollisionSys │        │  ShaderValidator  │      │  Hot-reloaded    │
└───────────────┘        │  MaterialManager  │      │  GLSL programs   │
                         └───────────────────┘      └──────────────────┘
```

## Modulární členění (`src/lib/`)

| Adresář | Moduly | Odpovědnost |
| --- | --- | --- |
| `Engine/` | `Window`, `Input`, `MazeApplication` | SDL3 okno, OpenGL 3.3 Core kontext, vstupní události, hlavní smyčka |
| `World/` | `MazeGenerator`, `ChunkManager`, `SectorManager`, `World` | Deterministická topologie bludiště, streaming chunků, definice sektorů |
| `Game/` | `Player`, `CollisionSystem` | First-person pohyb, substep 2D AABB vs kruh kolize nezávislé na shaderech |
| `Render/` | `GLFunctions`, `Camera`, `Renderer`, `DebugOverlay` | Dynamické načítání GL 3.3 funkcí, frustum culling, batching 1 draw call/chunk, 2D HUD |
| `Shader/` | `ShaderAPI`, `ShaderValidator`, `ShaderCache`, `ShaderManager`, `MaterialManager` | Šablona GLSL, bezpečnostní validace, disková cache, kompilace + FBO probe + hot reload |
| `AI/` | `AIProvider`, `MockAIProvider`, `OpenAICompatibleProvider`, `AIShaderService` | Abstrakce provideru, HTTP/JSON klient (`libcurl`), worker pool, předgenerování |
| `Utils/` | `UtilsFactory`, `Logger`, `Assets`, `Filesystem`, `Platform` | Infrastruktura zděděná z NixonCpp šablony |

## Svět a streaming

- Každý chunk obsahuje mřížku `8 × 8` průchozích buněk (`4.0 m × 4.0 m`, výška stěn `4.2 m`).
- `MazeGenerator` generuje každý chunk čistě z `(worldSeed, chunkX, chunkZ)`:
  1. Iterativní randomized DFS spanning tree zajistí 100% vnitřní průchodnost.
  2. ~16 % vnitřních stěn je vybouráno pro smyčky a lepší orientaci.
  3. Na každé ze 4 hranic chunku se deterministicky ze sdíleného hashe sousedních chunků otevřou 2 portály — oba sousedé tak vypočtou přesně shodné průchody bez nutnosti načítat sousední chunk.
- `ChunkManager` načítá chunky v poloměru `chunkLoadRadius` a uvolňuje vzdálené chunky (hystereze + hard limit `maxLoadedChunks`).
- `SectorManager` seskupuje `2 × 2` chunky do jednoho sektoru (`sector_<x>_<z>`) s deterministicky přiřazeným vizuálním archetypem.

## Životní cyklus shaderu

1. Hráč vstoupí do sektoru nebo stiskne `F2`.
2. `AIShaderService` zkontroluje `ShaderCache` (pokud není vyžádána regenerace).
3. Při cache miss zařadí požadavek do omezeného worker poolu (`maxParallelRequests`).
4. Odpověď AI se předá přes lock-guarded frontu hlavnímu (OpenGL) vláknu.
5. `ShaderValidator` provede strukturální kontrolu (zakázané direktivy, limity smyček, vyvážené závorky, podpis `evaluateSurface`).
6. `ShaderManager` vloží kód do pevné šablony, zkompiluje a slinkuje program a provede **kontrolovaný offscreen 16×16 FBO testovací probe** s měřením času a kontrolou `glGetError()`.
7. Teprve po úspěšném probe se shader uloží do verzované diskové cache a atomicky nahradí (hot reload). Při chybě zůstává aktivní poslední funkční shader nebo výchozí fallback.
