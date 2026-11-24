#pragma once

#include "AI/AIProvider.hpp"

namespace ailivingmaze::ai {

  class OpenAICompatibleProvider final : public IAIProvider {
  public:
    explicit OpenAICompatibleProvider(AIConfig config);

    [[nodiscard]] std::string name() const override { return "OpenAICompatibleProvider"; }
    [[nodiscard]] std::string modelName() const override { return config_.model; }
    [[nodiscard]] bool isAvailable() const override;
    [[nodiscard]] AIShaderResponse generateShader(const ShaderPromptContext &ctx) override;

    // Parses an OpenAI-compatible chat completion JSON response body (exposed for unit tests).
    [[nodiscard]] static bool extractGlslFromApiJson(const std::string &jsonBody,
                                                     std::string &glslOut, std::string &errorOut);

  private:
    [[nodiscard]] std::string readApiKeyFromEnv() const;
    [[nodiscard]] bool isLocalEndpoint() const;

    AIConfig config_;
  };

} // namespace ailivingmaze::ai
