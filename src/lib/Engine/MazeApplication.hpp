#pragma once

#include "AI/AIProvider.hpp"
#include "Utils/UtilsFactory.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace ailivingmaze::engine {

  struct EngineConfig {
    uint64_t worldSeed = 1337ULL;
    int chunkLoadRadius = 2;
    int windowWidth = 1280;
    int windowHeight = 720;
    bool vsync = true;
    bool invertMouseY = false;
    std::filesystem::path shaderCacheDir = ".cache/ailivingmaze/shaders";
    ai::AIConfig aiConfig{};
  };

  class MazeApplication {
  public:
    MazeApplication(const utils::UtilsFactory::ApplicationContext &context, EngineConfig config);

    // Runs the interactive SDL3 + OpenGL 3D loop. Returns EXIT_SUCCESS or EXIT_FAILURE.
    int run();

  private:
    std::shared_ptr<logging::ILogger> logger_;
    EngineConfig config_;
  };

} // namespace ailivingmaze::engine
