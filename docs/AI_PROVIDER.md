# AI Provider — Konfigurace a integrace

Subsystém `src/lib/AI/` poskytuje čisté rozhraní `IAIProvider`, díky němuž lze libovolně měnit zdroj shaderů bez zásahu do enginu.

## Dostupné providery

### 1. `MockAIProvider` (`--ai-provider mock`, výchozí)
- Plně offline, nulové náklady, nevyžaduje API klíč.
- Obsahuje 9 procedurálních GLSL archetypů odpovídajících tématům sektorů a reaguje na `variationSeed` při stisku `F2`.
- Simuluje krátkou asynchronní latenci (`60 ms`), aby bylo možné ověřit neblokující běh renderovací smyčky.

### 2. `OpenAICompatibleProvider` (`--ai-provider openai`)
- Komunikuje přes HTTP POST (`libcurl`) na libovolný OpenAI-compatible `/v1/chat/completions` endpoint.
- API klíč čte výhradně z proměnné prostředí (`--ai-key-env`, výchozí `OPENAI_API_KEY`).
- Pro lokální endpointy (`localhost` / `127.0.0.1`, např. Ollama, vLLM, llama.cpp server) funguje i bez nastaveného API klíče.
- Implementuje síťový timeout (`--ai-timeout`), omezený počet opakování (`maxRetries = 2`) a bezpečné očištění markdown code fences (` ```glsl ... ``` `).

### 3. Offline režim (`--ai-provider none`)
- Zakáže volání AI; engine používá výchozí vestavěný shader (případně již dříve nacachované validní shadery).

## Konfigurovatelné parametry

| CLI přepínač | Výchozí hodnota | Popis |
| --- | --- | --- |
| `--ai-provider` | `mock` | `mock`, `openai` nebo `none` |
| `--ai-endpoint` | `https://api.openai.com/v1/chat/completions` | URL OpenAI-compatible endpointu |
| `--ai-model` | `gpt-4o-mini` | Identifikátor modelu |
| `--ai-key-env` | `OPENAI_API_KEY` | Název proměnné prostředí s klíčem |
| `--ai-timeout` | `25` | Timeout HTTP požadavku (sekundy) |
| `--ai-parallel` | `2` | Max počet paralelních workerů |
| `--ai-pregen-radius` | `1` | Poloměr předgenerování okolních sektorů |
| `--cache-dir` | `.cache/ailivingmaze/shaders` | Adresář persistentní diskové cache |

## Struktura promptu

`buildStructuredShaderPrompt()` předává modelu:
- `sectorId` a `variationSeed`,
- název vizuálního stylu, popis atmosféry a vlastností materiálů,
- barevnou paletu (`primary`, `secondary`, `accent`),
- přesný kontrakt `SurfaceInput` / `SurfaceOutput` a seznam pomocných funkcí,
- výkonová a bezpečnostní omezení (bez textur, max 4 `for` smyčky ≤ 32 iterací, rozlišení podlaha/stěna/strop, animace pomocí `s.time`).
