#include "Shader/MaterialManager.hpp"

namespace ailivingmaze::shader {

  GLuint MaterialManager::resolveProgramForSector(const std::string &sectorId) const {
    if (useAIShaders_) {
      if (const auto *handle = shaderManager_.findSectorProgram(sectorId);
          handle != nullptr && handle->programId != 0) {
        return handle->programId;
      }
    }
    return shaderManager_.defaultProgram();
  }

} // namespace ailivingmaze::shader
