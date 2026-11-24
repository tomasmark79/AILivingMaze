#pragma once

#include "AI/AIProvider.hpp"

namespace ailivingmaze::ai {

  class MockAIProvider final : public IAIProvider {
  public:
    explicit MockAIProvider(int simulatedLatencyMs = 60) noexcept
        : simulatedLatencyMs_(simulatedLatencyMs) {}

    [[nodiscard]] std::string name() const override { return "MockAIProvider"; }
    [[nodiscard]] std::string modelName() const override { return "mock-procedural-v1"; }
    [[nodiscard]] bool isAvailable() const override { return true; }
    [[nodiscard]] AIShaderResponse generateShader(const ShaderPromptContext &ctx) override;

  private:
    int simulatedLatencyMs_ = 60;
  };

} // namespace ailivingmaze::ai
