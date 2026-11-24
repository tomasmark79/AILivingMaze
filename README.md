# AI Living Maze

Experimentální 3D engine v C++20 postavený na šabloně **NixonCpp** (Meson + Nix), ve kterém AI model za běhu generuje procedurální **GLSL** definice vizuálního prostředí, zatímco geometrii, kolize a vykreslování deterministicky řídí klasický OpenGL 3.3 Core engine.

> AI negeneruje jednotlivé snímky ani bitmapové textury — generuje programové definice povrchů (`evaluateSurface`) vykonávané přímo na GPU. Aplikace je plně funkční i zcela offline s vestavěným výchozím shaderem nebo s `MockAIProvider`.

## Klíčové vlastnosti MVP

- **Nekonečné procedurální 3D bludiště** — deterministické generování chunků z world seedu s matematicky garantovanou vnitřní průchodností i symetrickými portály na hranicích chunků.
- **Dynamický streaming chunků a sektorů** — načítání a uvolňování chunků podle pozice hráče, 9 architektonicko-vizuálních archetypů sektorů.
- **Klasický 3D renderer (SDL3 + OpenGL 3.3 Core + GLM)** — frustum culling, 1 draw call na chunk, RAII správa GPU prostředků, optimalizováno pro integrované GPU (např. Radeon 780M).
- **AI Shader Engine s hot reloadem** — stabilní verzované Shader API (`1.0.0`), strukturální validátor GLSL, offscreen 16×16 GPU testovací probe před aktivací, bezpečný fallback při chybě, persistentní disková cache.
- **Vyměnitelný AI provider** — `MockAIProvider` (výchozí pro offline vývoj a testování) i `OpenAICompatibleProvider` (libcurl, podpora cloudových i lokálních OpenAI-compatible API jako Ollama/vLLM/llama.cpp).
- **Diagnostický HUD (F1)** — vestavěný bitmap font bez externích fontů, FPS, frame time, pozice, chunk/sektor, draw calls, stav AI, časy generování/kompilace, cache statistiky a poslední chyba.

## Sestavení a spuštění na NixOS

Veškeré závislosti (`SDL3`, `OpenGL/libglvnd`, `GLM`, `libcurl`, `fmt`, `nlohmann_json`, `cxxopts`, `GTest`, `meson`, `ninja`) spravuje Nix flake v adresáři `nix/`.

```bash
# Vstup do reprodukovatelného vývojového prostředí
nix develop ./nix

# Debug sestavení
make debug

# Spuštění testů (GoogleTest)
make test

# Spuštění aplikace (výchozí MockAIProvider — bez API klíče)
./build/builddir-debug/AILivingMaze
```

Pro release sestavení:

```bash
make build
./build/builddir-release/AILivingMaze
```

## Konfigurace AI provideru

API klíče se nikdy neukládají do repozitáře — předávají se výhradně proměnnou prostředí (výchozí `OPENAI_API_KEY`).

```bash
# 1. Výchozí offline / vývojový režim s procedurálním MockAIProvider
./build/builddir-debug/AILivingMaze --ai-provider mock

# 2. Čistě offline režim bez AI (pouze vestavěný výchozí shader)
./build/builddir-debug/AILivingMaze --ai-provider none

# 3. OpenAI API
export OPENAI_API_KEY="sk-..."
./build/builddir-debug/AILivingMaze \
  --ai-provider openai \
  --ai-endpoint https://api.openai.com/v1/chat/completions \
  --ai-model gpt-4o-mini

# 4. Lokální OpenAI-compatible server (např. Ollama / llama.cpp) bez API klíče
./build/builddir-debug/AILivingMaze \
  --ai-provider openai \
  --ai-endpoint http://localhost:11434/v1/chat/completions \
  --ai-model qwen2.5-coder:7b
```

Všechny CLI volby zobrazí `./build/builddir-debug/AILivingMaze --help`.

## Ovládání

| Klávesa / vstup | Akce |
| --- | --- |
| `W` `A` `S` `D` / šipky | Pohyb v bludišti |
| `Myš` | Rozhlížení (first-person kamera) |
| `Shift` | Rychlejší pohyb (sprint) |
| `Escape` | Uvolnění / opětovné zachycení kurzoru myši (levé kliknutí kurzor znovu zachytí) |
| `F1` | Zapnutí / vypnutí diagnostického overlaye |
| `F2` | Okamžitý požadavek na nový AI shader pro aktuální sektor (hot reload) |
| `F3` | Přepnutí mezi AI-generovaným shaderem a výchozím fallback shaderem |
| `F4` | Zapnutí / vypnutí inverze vertikální osy myši (lze také zapnout parametrem `--invert-mouse-y`) |

## Dokumentace

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — modulární architektura, datový tok a životní cyklus shaderů
- [docs/SHADER_API.md](docs/SHADER_API.md) — stabilní GLSL kontrakt (`SurfaceInput` / `SurfaceOutput`), šablona a validace
- [docs/AI_PROVIDER.md](docs/AI_PROVIDER.md) — konfigurace providerů, prompt struktura a bezpečnostní zásady
- [docs/ROADMAP.md](docs/ROADMAP.md) — další plánované etapy po MVP

## Licence

[MIT](LICENSE)
