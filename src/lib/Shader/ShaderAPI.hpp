#pragma once

#include <string>
#include <string_view>

namespace ailivingmaze::shader {

  inline constexpr const char *kShaderApiVersion = "1.0.0";

#ifdef __EMSCRIPTEN__
#define AILIVINGMAZE_GLSL_HEADER "#version 300 es\nprecision highp float;\nprecision highp int;\n"
#else
#define AILIVINGMAZE_GLSL_HEADER "#version 330 core\n"
#endif

  inline constexpr const char *kFixedVertexShader = AILIVINGMAZE_GLSL_HEADER R"GLSL(

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec3 a_localPos;
layout(location = 4) in float a_surfaceType;

uniform mat4 u_viewProj;

out vec3 v_worldPos;
out vec3 v_localPos;
out vec3 v_normal;
out vec2 v_uv;
out float v_surfaceType;

void main() {
  v_worldPos = a_pos;
  v_localPos = a_localPos;
  v_normal = normalize(a_normal);
  v_uv = a_uv;
  v_surfaceType = a_surfaceType;
  gl_Position = u_viewProj * vec4(a_pos, 1.0);
}
)GLSL";

  inline constexpr const char *kFragmentTemplateHeader = AILIVINGMAZE_GLSL_HEADER R"GLSL(

in vec3 v_worldPos;
in vec3 v_localPos;
in vec3 v_normal;
in vec2 v_uv;
in float v_surfaceType;

uniform float u_time;
uniform vec3 u_cameraPos;
uniform vec2 u_sectorCoord;
uniform int u_sectorHash;
uniform vec3 u_primaryColor;
uniform vec3 u_secondaryColor;
uniform vec3 u_accentColor;
uniform vec3 u_fogColor;
uniform float u_fogDensity;
uniform float u_lightIntensity;
uniform vec3 u_lightDir;

out vec4 FragColor;

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

// Built-in safe procedural helpers available to AI-generated shaders
float lm_hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float lm_noise2D(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = lm_hash21(i);
  float b = lm_hash21(i + vec2(1.0, 0.0));
  float c = lm_hash21(i + vec2(0.0, 1.0));
  float d = lm_hash21(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float lm_fbm2D(vec2 p) {
  float value = 0.0;
  float amp = 0.5;
  for (int i = 0; i < 4; ++i) {
    value += amp * lm_noise2D(p);
    p *= 2.03;
    amp *= 0.5;
  }
  return value;
}

// --- BEGIN AI GENERATED SURFACE FUNCTION ---
)GLSL";

  inline constexpr const char *kFragmentTemplateFooter = R"GLSL(
// --- END AI GENERATED SURFACE FUNCTION ---

void main() {
  vec3 N = normalize(v_normal);
  vec3 toCam = u_cameraPos - v_worldPos;
  float dist = length(toCam);
  vec3 V = dist > 0.0001 ? (toCam / dist) : vec3(0.0, 0.0, 1.0);

  SurfaceInput surf;
  surf.worldPos = v_worldPos;
  surf.localPos = v_localPos;
  surf.uv = v_uv;
  surf.normal = N;
  surf.viewDir = V;
  surf.time = u_time;
  surf.surfaceType = v_surfaceType;
  surf.sectorCoord = u_sectorCoord;
  surf.sectorHash = u_sectorHash;
  surf.primaryColor = u_primaryColor;
  surf.secondaryColor = u_secondaryColor;
  surf.accentColor = u_accentColor;

  SurfaceOutput mat = evaluateSurface(surf);
  vec3 albedo = clamp(mat.albedo, vec3(0.0), vec3(1.0));
  vec3 emissive = clamp(mat.emissive, vec3(0.0), vec3(4.0));
  float roughness = clamp(mat.roughness, 0.08, 1.0);
  float metallic = clamp(mat.metallic, 0.0, 1.0);
  float ao = clamp(mat.ao, 0.1, 1.0);

  // Subtle architectural hemisphere + directional light
  vec3 Ldir = normalize(u_lightDir);
  float ndl = max(dot(N, Ldir), 0.0);
  vec3 Hdir = normalize(Ldir + V);
  float specPow = mix(64.0, 4.0, roughness);
  float specDir = pow(max(dot(N, Hdir), 0.0), specPow) * mix(0.15, 0.85, metallic);

  // Player-carried atmospheric lantern light for maze readability
  float atten = 1.0 / (1.0 + 0.025 * dist + 0.004 * dist * dist);
  float ndv = max(dot(N, V), 0.0);
  float specLamp = pow(ndv, specPow * 0.6) * 0.25;

  vec3 ambient = mix(u_secondaryColor, u_primaryColor, 0.25) * 0.35 * ao;
  vec3 direct = (albedo * (0.55 * ndl + 0.95 * ndv * atten) +
                 mix(vec3(1.0), albedo, metallic) * (specDir * 0.4 + specLamp * atten)) *
                u_lightIntensity;

  vec3 color = ambient * albedo + direct + emissive;

  // Exponential distance fog for mysterious scale and smooth sector transitions
  float fogFactor = 1.0 - exp(-pow(dist * u_fogDensity, 1.45));
  fogFactor = clamp(fogFactor, 0.0, 0.96);
  color = mix(color, u_fogColor, fogFactor);

  // Filmic-like tone mapping + gamma
  color = color / (color + vec3(1.0));
  color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
  FragColor = vec4(color, 1.0);
}
)GLSL";

  inline constexpr const char *kDefaultSurfaceGlsl = R"GLSL(
SurfaceOutput evaluateSurface(SurfaceInput s) {
  SurfaceOutput o;
  vec2 grid = abs(fract(s.uv * 2.0 - 0.5) - 0.5) / 0.06;
  float line = 1.0 - min(min(grid.x, grid.y), 1.0);
  float n = lm_fbm2D(s.worldPos.xz * 0.35 + vec2(s.worldPos.y * 0.25));

  if (s.surfaceType < 0.5) {
    // Floor
    o.albedo = mix(s.secondaryColor, s.primaryColor * 0.55, n * 0.5);
    o.emissive = s.accentColor * line * 0.18;
    o.roughness = 0.45;
    o.metallic = 0.25;
    o.ao = 0.85 + 0.15 * n;
  } else if (s.surfaceType < 1.5) {
    // Wall
    float band = smoothstep(0.46, 0.50, abs(fract(s.worldPos.y * 0.5) - 0.5));
    float pulse = 0.5 + 0.5 * sin(s.time * 1.4 + s.worldPos.x * 0.4 + s.worldPos.z * 0.4);
    o.albedo = mix(s.secondaryColor * 1.15, s.primaryColor * 0.75, n * 0.65);
    o.emissive = s.accentColor * (line * 0.22 + band * 0.35 * pulse);
    o.roughness = 0.35;
    o.metallic = 0.35;
    o.ao = 0.9;
  } else {
    // Ceiling
    float ribs = smoothstep(0.35, 0.48, abs(fract(s.uv.x * 2.0) - 0.5));
    o.albedo = s.secondaryColor * 0.8;
    o.emissive = s.primaryColor * ribs * 0.25;
    o.roughness = 0.6;
    o.metallic = 0.2;
    o.ao = 0.8;
  }
  return o;
}
)GLSL";

  inline std::string composeFragmentShader(std::string_view surfaceGlsl) {
    std::string out;
    out.reserve(std::char_traits<char>::length(kFragmentTemplateHeader) + surfaceGlsl.size() +
                std::char_traits<char>::length(kFragmentTemplateFooter) + 4);
    out.append(kFragmentTemplateHeader);
    out.push_back('\n');
    out.append(surfaceGlsl);
    out.push_back('\n');
    out.append(kFragmentTemplateFooter);
    return out;
  }

} // namespace ailivingmaze::shader
