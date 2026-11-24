# Stabilní Shader API (`v1.0.0`)

AI Living Maze odděluje fixní renderovací pipeline enginu od dynamicky generovaného kódu povrchů. AI model **nesmí** definovat `#version`, vlastní `uniform`, `layout(...)`, ani `void main()`.

## Vertex shader (pevný)

Engine používá pevný vertex shader (`kFixedVertexShader` v `src/lib/Shader/ShaderAPI.hpp`), který předává fragment shaderu:

- `v_worldPos` (`vec3`) — světové souřadnice vrcholu
- `v_localPos` (`vec3`) — souřadnice lokální vůči počátku chunku
- `v_normal` (`vec3`) — normalizovaná světová normála
- `v_uv` (`vec2`) — UV souřadnice povrchu
- `v_surfaceType` (`float`) — `0.0` podlaha, `1.0` stěna, `2.0` strop

## Fragment shader kontrakt

Engine před vložením AI kódu deklaruje struktury a vestavěné pomocné funkce:

```glsl
struct SurfaceInput {
  vec3 worldPos;
  vec3 localPos;
  vec2 uv;
  vec3 normal;
  vec3 viewDir;
  float time;
  float surfaceType; // 0.0 = floor, 1.0 = wall, 2.0 = ceiling
  vec2 sectorCoord;
  int sectorHash;
  vec3 primaryColor;
  vec3 secondaryColor;
  vec3 accentColor;
};

struct SurfaceOutput {
  vec3 albedo;
  vec3 emissive;
  float roughness;
  float metallic;
  float ao;
};
```

Dostupné pomocné funkce:

- `float lm_hash21(vec2 p)`
- `float lm_noise2D(vec2 p)`
- `float lm_fbm2D(vec2 p)`

### Povinný vstupní bod AI kódu

AI kód musí implementovat právě jednu vstupní funkci (případně vlastní čisté pomocné funkce nad ní):

```glsl
SurfaceOutput evaluateSurface(SurfaceInput s)
```

Pevná `void main()` v šabloně následně:
1. Omezí výstupy `albedo`, `emissive`, `roughness`, `metallic`, `ao` do bezpečných rozsahů.
2. Vypočte směrové světlo, atmosférické světlo hráče, spekulární odlesky a ambientní složku.
3. Aplikuje exponenciální mlhu sektoru pro plynulé přechody a prostorovou hloubku.
4. Provede tone mapping a gamma korekci.

## Pravidla strukturální validace (`ShaderValidator`)

Před kompilací v OpenGL musí kód splnit:
- Délka mezi `30 B` a `16 KB`.
- Vyvážené závorky `{}` a `()`.
- Přítomnost podpisu `SurfaceOutput evaluateSurface(SurfaceInput <ident>)`.
- Zákaz nebezpečných/pipeline tokenů: `#version`, `#extension`, `#include`, `layout(`, `uniform `, `void main`, `discard;`, `gl_FragDepth`, `imageStore`, `while`.
- Maximálně `4` smyčky `for`, každá s literálovou mezí nejvýše `32` iterací.

## Verzování cache

Každý uložený shader v diskové cache nese pole `shaderApiVersion` (aktuálně `"1.0.0"`) a hash obsahu. Při změně verze API jsou staré záznamy automaticky ignorovány.
