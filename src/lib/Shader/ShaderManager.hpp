#pragma once

#include "Render/GLFunctions.hpp"
#include "Shader/ShaderCache.hpp"

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

namespace ailivingmaze::shader {

  struct ShaderProgramHandle {
    GLuint programId = 0;
    std::string sectorId;
    std::string shaderHash;
    std::string aiModel;
    double compileMs = 0.0;
    double probeMs = 0.0;
    bool isAI = false;
  };

  struct ShaderCompileResult {
    bool success = false;
    double compileMs = 0.0;
    double probeMs = 0.0;
    std::string errorMessage;
  };

  class ShaderManager {
  public:
    explicit ShaderManager(std::size_t maxActivePrograms = 24);
    ~ShaderManager();

    ShaderManager(const ShaderManager &) = delete;
    ShaderManager &operator=(const ShaderManager &) = delete;

    // Must be called on the OpenGL thread after GL functions are loaded.
    bool init(std::string &errorOut);
    void shutdown();

    // Validates, compiles, links, runs offscreen test probe, and hot-reloads sector shader.
    ShaderCompileResult compileAndActivate(const std::string &sectorId,
                                           const std::string &surfaceGlsl,
                                           const std::string &aiModel,
                                           ShaderCache *cache = nullptr,
                                           const std::string &prompt = "",
                                           double generationMs = 0.0);

    [[nodiscard]] GLuint defaultProgram() const noexcept { return defaultProgram_; }
    [[nodiscard]] const ShaderProgramHandle *findSectorProgram(const std::string &sectorId);
    [[nodiscard]] bool hasSectorProgram(const std::string &sectorId) const noexcept {
      return programs_.find(sectorId) != programs_.end();
    }

    [[nodiscard]] std::size_t activeProgramCount() const noexcept {
      return programs_.size() + (defaultProgram_ != 0 ? 1 : 0);
    }
    [[nodiscard]] double lastCompileMs() const noexcept { return lastCompileMs_; }
    [[nodiscard]] const std::string &lastError() const noexcept { return lastError_; }

  private:
    bool compileStage(GLenum type, const std::string &source, GLuint &shaderOut,
                      std::string &errorOut);
    bool buildProgram(const std::string &fragmentSource, GLuint &programOut, double &compileMsOut,
                      std::string &errorOut);
    bool runControlledProbe(GLuint program, double &probeMsOut, std::string &errorOut);
    void evictIfNeeded();

    std::size_t maxActivePrograms_ = 24;
    GLuint defaultProgram_ = 0;
    GLuint probeFbo_ = 0;
    GLuint probeTex_ = 0;
    GLuint probeVao_ = 0;
    GLuint probeVbo_ = 0;

    std::unordered_map<std::string, ShaderProgramHandle> programs_;
    std::list<std::string> lruOrder_;
    double lastCompileMs_ = 0.0;
    std::string lastError_;
  };

} // namespace ailivingmaze::shader
