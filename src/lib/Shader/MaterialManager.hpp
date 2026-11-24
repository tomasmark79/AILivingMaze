#pragma once

#include "Shader/ShaderManager.hpp"

#include <string>

namespace ailivingmaze::shader {

  class MaterialManager {
  public:
    explicit MaterialManager(ShaderManager &shaderManager) noexcept
        : shaderManager_(shaderManager) {}

    void setUseAIShaders(bool enabled) noexcept { useAIShaders_ = enabled; }
    void toggleShaderMode() noexcept { useAIShaders_ = !useAIShaders_; }
    [[nodiscard]] bool usingAIShaders() const noexcept { return useAIShaders_; }

    [[nodiscard]] GLuint resolveProgramForSector(const std::string &sectorId) const;

  private:
    ShaderManager &shaderManager_;
    bool useAIShaders_ = true;
  };

} // namespace ailivingmaze::shader
