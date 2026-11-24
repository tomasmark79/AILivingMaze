# Roadmapa — AI Living Maze

## Dokončeno (MVP v0.1)

- [x] Základ na šabloně NixonCpp (C++20, Meson, Nix flake pro NixOS)
- [x] SDL3 okno, OpenGL 3.3 Core loader, FPS kamera a ovládání (WASD, Shift, myš, ESC, F1–F3)
- [x] Deterministický procedurální generátor nekonečného bludiště s garantovanou průchodností
- [x] Dynamický `ChunkManager`, `SectorManager` (9 vizuálních archetypů) a nezávislý `CollisionSystem`
- [x] Stabilní Shader API (`v1.0.0`), `ShaderValidator`, offscreen FBO probe a hot reload
- [x] Verzovaná persistentní disková `ShaderCache`
- [x] `MockAIProvider`, `OpenAICompatibleProvider` (`libcurl`) a asynchronní `AIShaderService` s předgenerováním
- [x] Diagnostický HUD (`DebugOverlay`, F1) a GoogleTest sada

## Další etapy (Post-MVP)

### v0.2 — Plynulé prolínání sektorů a profilování GPU
- Plynulý cross-fade mezi shadery sousedních sektorů na hranicích (multi-pass nebo dual-evaluation přechodová zóna).
- OpenGL timer queries (`GL_TIME_QUERY`) pro přesné měření času fragment shaderu na GPU a automatický downgrade pomalých shaderů.

### v0.3 — Architektonické variace geometrie
- Víceúrovňové sály, sloupy, oblouky a změny výšky stropů řízené metadaty sektoru.
- Volitelné procedurální ambientní audio přes SDL3 Audio stream generované podle atmosféry sektoru.

### v0.4 — Pokročilý AI Shader Pipeline
- Iterativní self-healing: při chybě GLSL kompilace poslat diagnostiku zpět AI modelu (s limitem 1–2 opravy).
- Export/import kurátorovaných sad sektorových shaderů a screenshot galerie.
