#pragma once

#include "Shader/ShaderAPI.hpp"
#include "World/SectorManager.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace ailivingmaze::ai {

  struct AIConfig {
    bool enabled = true;
    std::string providerType = "mock"; // "mock", "openai", "none"
    std::string endpoint = "https://api.openai.com/v1/chat/completions";
    std::string model = "gpt-4o-mini";
    std::string apiKeyEnvVar = "OPENAI_API_KEY";
    int timeoutSeconds = 25;
    int maxRetries = 2;
    int maxParallelRequests = 2;
    int pregenerateRadius = 1;
    int mockLatencyMs = 60;
  };

  struct ShaderPromptContext {
    world::SectorTheme theme{};
    uint32_t variationSeed = 0;
  };

  struct AIShaderResponse {
    bool success = false;
    std::string sectorId;
    std::string glslCode;
    std::string promptUsed;
    std::string modelUsed;
    double generationMs = 0.0;
    bool fromCache = false;
    std::string errorMessage;
  };

  class IAIProvider {
  public:
    virtual ~IAIProvider() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::string modelName() const = 0;
    [[nodiscard]] virtual bool isAvailable() const = 0;
    [[nodiscard]] virtual AIShaderResponse generateShader(const ShaderPromptContext &ctx) = 0;
  };

  inline std::string buildStructuredShaderPrompt(const ShaderPromptContext &ctx) {
    auto fmtVec = [](const glm::vec3 &v) {
      std::ostringstream s;
      s << std::fixed << std::setprecision(2) << "vec3(" << v.r << ", " << v.g << ", " << v.b
        << ")";
      return s.str();
    };

    std::ostringstream oss;
    oss << "Generate a GLSL 330 procedural surface function for AI Living Maze.\n"
        << "Shader API version: " << shader::kShaderApiVersion << "\n"
        << "Sector ID: " << ctx.theme.sectorId << "\n"
        << "Variation Seed: " << ctx.variationSeed << "\n"
        << "Visual Style: " << ctx.theme.styleName << "\n"
        << "Atmosphere: " << ctx.theme.atmosphere << "\n"
        << "Material Properties: " << ctx.theme.materialProperties << "\n"
        << "Palette: primary=" << fmtVec(ctx.theme.primaryColor)
        << ", secondary=" << fmtVec(ctx.theme.secondaryColor)
        << ", accent=" << fmtVec(ctx.theme.accentColor) << "\n\n"
        << "REQUIRED INTERFACE (do not redeclare structs, uniforms, #version, or main()):\n"
        << "  SurfaceOutput evaluateSurface(SurfaceInput s)\n"
        << "Available SurfaceInput fields:\n"
        << "  vec3 worldPos, vec3 localPos, vec2 uv, vec3 normal, vec3 viewDir,\n"
        << "  float time, float surfaceType (0=floor, 1=wall, 2=ceiling),\n"
        << "  vec2 sectorCoord, int sectorHash, vec3 primaryColor, vec3 secondaryColor, vec3 accentColor\n"
        << "SurfaceOutput fields to populate:\n"
        << "  vec3 albedo, vec3 emissive, float roughness, float metallic, float ao\n"
        << "Available built-in helpers: float lm_hash21(vec2), float lm_noise2D(vec2), float lm_fbm2D(vec2)\n"
        << "CONSTRAINTS:\n"
        << "- Procedural GLSL only (no textures, no uniforms, no layout, no discard, no while loops).\n"
        << "- At most 4 for-loops, each <= 32 iterations. Fast on integrated GPUs.\n"
        << "- Differentiate floor (surfaceType < 0.5), walls (0.5..1.5), and ceiling (> 1.5).\n"
        << "- Include subtle animated emissive details using s.time.\n"
        << "- Return ONLY valid GLSL code defining helper functions (if any) and evaluateSurface.\n";
    return oss.str();
  }

} // namespace ailivingmaze::ai
