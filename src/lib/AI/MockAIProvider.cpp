#include "AI/MockAIProvider.hpp"

#include <array>
#include <chrono>
#include <sstream>
#include <thread>

namespace ailivingmaze::ai {

  namespace {
    // 9 distinct procedural GLSL archetypes matching SectorManager presets.
    // Variation seed shifts frequency, speed, and phase so F2 produces visibly new results.
    std::string buildMockShader(int archetype, uint32_t variationSeed) {
      const float freqScale = 1.0F + 0.28F * static_cast<float>(variationSeed % 5U);
      const float speedScale = 0.85F + 0.35F * static_cast<float>((variationSeed / 2U) % 4U);
      const float phase = 1.37F * static_cast<float>(variationSeed);

      std::ostringstream oss;
      oss << "// MockAIProvider archetype=" << archetype << " var=" << variationSeed << "\n";
      oss << "const float kFreq = " << freqScale << ";\n";
      oss << "const float kSpeed = " << speedScale << ";\n";
      oss << "const float kPhase = " << phase << ";\n\n";

      switch (archetype % 9) {
        case 0: // Futuristic Tech Corridors
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  vec2 p = s.uv * (2.5 * kFreq);
  vec2 cell = fract(p);
  vec2 id = floor(p);
  float edge = min(min(cell.x, 1.0 - cell.x), min(cell.y, 1.0 - cell.y));
  float panelLine = 1.0 - smoothstep(0.02, 0.055, edge);
  float trace = smoothstep(0.46, 0.50, abs(sin(s.worldPos.x * 1.4 * kFreq + s.worldPos.z * 1.4 * kFreq + kPhase)));
  float scan = 0.5 + 0.5 * sin(s.time * 2.2 * kSpeed - s.worldPos.y * 2.5 + lm_hash21(id) * 6.28);

  if (s.surfaceType < 0.5) {
    float hexGrid = abs(fract(s.worldPos.x * 0.5 * kFreq) - 0.5) + abs(fract(s.worldPos.z * 0.5 * kFreq) - 0.5);
    float strip = smoothstep(0.42, 0.48, hexGrid);
    o.albedo = mix(s.secondaryColor, s.primaryColor * 0.35, strip);
    o.emissive = s.accentColor * (1.0 - strip) * 0.25 * scan;
    o.roughness = 0.35;
    o.metallic = 0.6;
    o.ao = 0.9;
  } else if (s.surfaceType < 1.5) {
    float conduit = smoothstep(0.06, 0.01, abs(fract(s.worldPos.y * 0.45) - 0.5));
    o.albedo = mix(s.secondaryColor * 1.3, s.primaryColor * 0.45, lm_noise2D(s.worldPos.xz * 0.6));
    o.emissive = s.accentColor * (panelLine * 0.35 + conduit * 0.85 * scan + trace * 0.12);
    o.roughness = 0.28;
    o.metallic = 0.72;
    o.ao = 0.92;
  } else {
    float lightBar = smoothstep(0.18, 0.04, abs(fract(s.uv.x * kFreq) - 0.5));
    o.albedo = s.secondaryColor * 0.7;
    o.emissive = s.primaryColor * lightBar * (0.55 + 0.25 * scan);
    o.roughness = 0.4;
    o.metallic = 0.5;
    o.ao = 0.85;
  }
  return o;
}
)GLSL";
          break;

        case 1: // Organic Pulsing Walls
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  vec2 q = (s.surfaceType < 0.5 || s.surfaceType > 1.5) ? s.worldPos.xz : vec2(s.worldPos.x + s.worldPos.z, s.worldPos.y);
  float t = s.time * kSpeed + kPhase;
  float warp = lm_fbm2D(q * 0.55 * kFreq + vec2(sin(t * 0.4), cos(t * 0.35)) * 0.4);
  float veins = abs(sin((q.x + warp * 2.4) * 3.0 * kFreq) * cos((q.y - warp * 2.0) * 3.0 * kFreq));
  float veinGlow = smoothstep(0.08, 0.0, veins);
  float heartbeat = pow(0.5 + 0.5 * sin(t * 2.6 + warp * 4.0), 3.0);

  if (s.surfaceType < 0.5) {
    o.albedo = mix(s.secondaryColor, s.primaryColor * 0.5, warp);
    o.emissive = s.accentColor * veinGlow * 0.45 * heartbeat;
    o.roughness = 0.22;
    o.metallic = 0.05;
    o.ao = 0.75 + 0.25 * warp;
  } else if (s.surfaceType < 1.5) {
    o.albedo = mix(s.secondaryColor * 0.9, s.primaryColor * 0.85, warp);
    o.emissive = mix(s.primaryColor, s.accentColor, heartbeat) * (veinGlow * 0.95 + 0.12 * heartbeat);
    o.roughness = 0.28;
    o.metallic = 0.08;
    o.ao = 0.7 + 0.3 * warp;
  } else {
    o.albedo = s.secondaryColor * 0.65;
    o.emissive = s.accentColor * veinGlow * 0.6 * (0.4 + 0.6 * heartbeat);
    o.roughness = 0.35;
    o.metallic = 0.05;
    o.ao = 0.75;
  }
  return o;
}
)GLSL";
          break;

        case 2: // Crystalline Structures
          oss << R"GLSL(
float lm_voronoiEdge(vec2 x) {
  vec2 n = floor(x);
  vec2 f = fract(x);
  float md = 8.0;
  for (int j = -1; j <= 1; ++j) {
    for (int i = -1; i <= 1; ++i) {
      vec2 g = vec2(float(i), float(j));
      vec2 o = vec2(lm_hash21(n + g), lm_hash21(n + g + vec2(17.3, 9.1)));
      vec2 r = g + o - f;
      float d = dot(r, r);
      md = min(md, d);
    }
  }
  return sqrt(md);
}

SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  vec2 p = s.uv * (2.6 * kFreq) + vec2(kPhase * 0.1);
  float v = lm_voronoiEdge(p);
  float facet = smoothstep(0.15, 0.75, v);
  float rim = pow(1.0 - max(dot(s.normal, s.viewDir), 0.0), 2.5);
  float shimmer = 0.5 + 0.5 * sin(s.time * 1.8 * kSpeed + v * 12.0 + s.worldPos.x);

  o.albedo = mix(s.secondaryColor, s.primaryColor, facet);
  o.emissive = mix(s.primaryColor, s.accentColor, shimmer) * (rim * 0.55 + smoothstep(0.68, 0.85, v) * 0.45);
  o.roughness = 0.14;
  o.metallic = 0.45;
  o.ao = 0.85 + 0.15 * facet;
  if (s.surfaceType < 0.5) {
    o.albedo *= 0.7;
    o.roughness = 0.20;
  }
  return o;
}
)GLSL";
          break;

        case 3: // Dark Industrial Vault
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  float rust = lm_fbm2D(s.worldPos.xz * 0.8 * kFreq + vec2(s.worldPos.y * 0.9, kPhase));
  vec2 plate = fract(s.uv * (2.0 * kFreq));
  float seam = 1.0 - smoothstep(0.02, 0.05, min(min(plate.x, 1.0 - plate.x), min(plate.y, 1.0 - plate.y)));
  float furnace = 0.55 + 0.45 * sin(s.time * 1.5 * kSpeed + s.worldPos.x * 0.35 + s.worldPos.z * 0.35);

  if (s.surfaceType < 0.5) {
    float grate = step(0.18, fract(s.uv.x * 6.0 * kFreq)) * step(0.18, fract(s.uv.y * 6.0 * kFreq));
    o.albedo = mix(s.secondaryColor * 0.7, s.primaryColor * 0.55, grate);
    o.emissive = s.accentColor * (1.0 - grate) * 0.38 * furnace;
    o.roughness = 0.65;
    o.metallic = 0.75;
    o.ao = 0.85;
  } else if (s.surfaceType < 1.5) {
    float hazardBand = step(0.10, s.worldPos.y) * step(s.worldPos.y, 0.55);
    float stripe = step(0.5, fract((s.worldPos.x + s.worldPos.z) * 1.5 + s.worldPos.y * 1.5));
    vec3 baseCol = mix(s.secondaryColor, s.primaryColor * 0.6, rust);
    o.albedo = mix(baseCol, mix(vec3(0.08), s.accentColor, stripe), hazardBand * 0.85);
    float vent = smoothstep(0.08, 0.01, abs(s.worldPos.y - 2.6)) * step(0.45, fract(s.uv.x * 3.0));
    o.emissive = s.accentColor * (vent * 0.95 * furnace + seam * 0.08);
    o.roughness = 0.55 + 0.25 * rust;
    o.metallic = 0.65;
    o.ao = 0.85 - 0.2 * seam;
  } else {
    o.albedo = s.secondaryColor * 0.6;
    o.emissive = s.accentColor * seam * 0.25 * furnace;
    o.roughness = 0.7;
    o.metallic = 0.6;
    o.ao = 0.8;
  }
  return o;
}
)GLSL";
          break;

        case 4: // Abstract Mathematical Patterns
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  vec2 p = (s.uv - 0.5) * (6.28318 * kFreq);
  float t = s.time * kSpeed + kPhase;
  float wave = sin(p.x + sin(p.y + t * 0.7)) + cos(p.y - cos(p.x - t * 0.6));
  float contour = smoothstep(0.16, 0.03, abs(fract(wave * 0.65) - 0.5));
  float polar = sin(length(p) * 2.0 - t * 1.5);
  float rings = smoothstep(0.88, 0.98, abs(polar));

  o.albedo = mix(s.secondaryColor, s.primaryColor * 0.6, 0.5 + 0.5 * sin(wave));
  o.emissive = s.primaryColor * contour * 0.55 + s.accentColor * rings * 0.65;
  o.roughness = 0.3;
  o.metallic = 0.35;
  o.ao = 0.9;
  if (s.surfaceType < 0.5) {
    o.emissive *= 0.6;
  }
  return o;
}
)GLSL";
          break;

        case 5: // Alien Xenomaterials
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  vec2 p = s.uv * (3.2 * kFreq);
  p.x += step(1.0, mod(p.y, 2.0)) * 0.5;
  vec2 f = fract(p) - 0.5;
  float hex = max(abs(f.x) * 1.15, abs(f.y) + abs(f.x) * 0.5);
  float scaleEdge = smoothstep(0.42, 0.49, hex);
  float glyph = lm_noise2D(floor(p) * 1.7 + vec2(kPhase));
  float pulse = 0.5 + 0.5 * sin(s.time * 2.0 * kSpeed + glyph * 12.0);

  o.albedo = mix(s.secondaryColor * 0.9, s.primaryColor * 0.5, 1.0 - scaleEdge);
  o.emissive = mix(s.primaryColor, s.accentColor, pulse) * (scaleEdge * 0.75 + step(0.82, glyph) * pulse * 0.45);
  o.roughness = 0.25;
  o.metallic = 0.55;
  o.ao = 0.88;
  return o;
}
)GLSL";
          break;

        case 6: // Energy Lattice Structures
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  float t = s.time * 2.4 * kSpeed + kPhase;
  vec2 g = abs(fract(s.uv * (3.0 * kFreq) - 0.5) - 0.5);
  float lattice = smoothstep(0.07, 0.015, min(g.x, g.y));
  float arc = smoothstep(0.92, 0.99, sin(s.worldPos.x * 2.2 * kFreq + t) * cos(s.worldPos.z * 2.2 * kFreq - t * 0.8));
  float waveY = 0.5 + 0.5 * sin(s.worldPos.y * 3.5 - t * 1.6);

  o.albedo = s.secondaryColor * 1.1;
  o.emissive = s.primaryColor * lattice * (0.45 + 0.45 * waveY) + s.accentColor * arc * 1.3;
  o.roughness = 0.18;
  o.metallic = 0.82;
  o.ao = 0.95;
  return o;
}
)GLSL";
          break;

        case 7: // Surreal Dreamscape
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  float t = s.time * 0.9 * kSpeed + kPhase;
  vec2 p = s.worldPos.xz * 0.35 * kFreq + vec2(s.worldPos.y * 0.4);
  float ripple = sin(length(fract(p) - 0.5) * 14.0 - t * 2.0);
  float checker = mod(floor(s.uv.x * 3.0 * kFreq) + floor(s.uv.y * 3.0 * kFreq), 2.0);
  float aurora = 0.5 + 0.5 * sin(s.worldPos.y * 1.2 + s.worldPos.x * 0.25 + t);

  vec3 colA = mix(s.secondaryColor, s.primaryColor, aurora);
  vec3 colB = mix(s.primaryColor * 0.7, s.accentColor, 0.5 + 0.5 * ripple);
  o.albedo = mix(colA, colB, checker * 0.65);
  o.emissive = mix(s.primaryColor, s.accentColor, aurora) * smoothstep(0.65, 0.95, ripple) * 0.42;
  o.roughness = 0.24;
  o.metallic = 0.2;
  o.ao = 0.92;
  return o;
}
)GLSL";
          break;

        default: // Minimalist Monumental Halls
          oss << R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  float grain = lm_fbm2D(s.worldPos.xz * 1.1 * kFreq + vec2(s.worldPos.y * 0.6, kPhase));
  float jointY = smoothstep(0.025, 0.0, abs(fract(s.worldPos.y * 0.5) - 0.5));
  float slit = smoothstep(0.05, 0.008, abs(fract(s.uv.x * kFreq) - 0.5));
  float breathe = 0.75 + 0.25 * sin(s.time * 0.8 * kSpeed + s.worldPos.x * 0.15);

  if (s.surfaceType < 0.5) {
    o.albedo = mix(s.secondaryColor * 1.4, s.primaryColor * 0.65, grain * 0.4);
    o.emissive = vec3(0.0);
    o.roughness = 0.48;
    o.metallic = 0.08;
    o.ao = 0.92;
  } else if (s.surfaceType < 1.5) {
    o.albedo = mix(s.primaryColor * 0.72, s.primaryColor * 0.95, grain * 0.5) * (1.0 - 0.25 * jointY);
    o.emissive = s.accentColor * slit * 0.65 * breathe;
    o.roughness = 0.58;
    o.metallic = 0.05;
    o.ao = 0.92 - 0.18 * jointY;
  } else {
    o.albedo = s.primaryColor * 0.6;
    o.emissive = s.accentColor * slit * 0.8 * breathe;
    o.roughness = 0.65;
    o.metallic = 0.05;
    o.ao = 0.88;
  }
  return o;
}
)GLSL";
          break;
      }

      return oss.str();
    }
  } // namespace

  AIShaderResponse MockAIProvider::generateShader(const ShaderPromptContext &ctx) {
    const auto t0 = std::chrono::steady_clock::now();
#ifndef __EMSCRIPTEN__
    if (simulatedLatencyMs_ > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(simulatedLatencyMs_));
    }
#else
    (void)simulatedLatencyMs_;
#endif

    AIShaderResponse res{};
    res.sectorId = ctx.theme.sectorId;
    res.promptUsed = buildStructuredShaderPrompt(ctx);
    res.modelUsed = modelName();
    res.glslCode = buildMockShader(ctx.theme.archetypeIndex, ctx.variationSeed);
    res.success = true;

    const auto t1 = std::chrono::steady_clock::now();
    res.generationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
  }

} // namespace ailivingmaze::ai
