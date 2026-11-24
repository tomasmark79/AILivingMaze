#include "Shader/ShaderManager.hpp"

#include "Shader/ShaderAPI.hpp"
#include "Shader/ShaderValidator.hpp"

#include <chrono>
#include <vector>

namespace ailivingmaze::shader {

  ShaderManager::ShaderManager(std::size_t maxActivePrograms)
      : maxActivePrograms_(maxActivePrograms) {}

  ShaderManager::~ShaderManager() { shutdown(); }

  bool ShaderManager::init(std::string &errorOut) {
    auto &g = render::gl();
    if (!g.loaded) {
      errorOut = "OpenGL functions are not loaded";
      return false;
    }

    // Create 16x16 offscreen probe FBO and single triangle VBO for safe shader verification
    glGenTextures(1, &probeTex_);
    glBindTexture(GL_TEXTURE_2D, probeTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    g.GenFramebuffers(1, &probeFbo_);
    g.BindFramebuffer(GL_FRAMEBUFFER, probeFbo_);
    g.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, probeTex_, 0);
    if (g.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      g.BindFramebuffer(GL_FRAMEBUFFER, 0);
      errorOut = "Failed to initialize shader probe framebuffer";
      return false;
    }
    g.BindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3 vertices matching MeshVertex layout (pos3, norm3, uv2, local3, surf1 = 12 floats)
    const float probeVerts[3 * 12] = {
        -1.0F, -1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
        3.0F,  -1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 2.0F, 0.0F, 2.0F, 0.0F, 0.0F, 1.0F,
        -1.0F, 3.0F,  0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 2.0F, 0.0F, 2.0F, 0.0F, 1.0F,
    };
    g.GenVertexArrays(1, &probeVao_);
    g.GenBuffers(1, &probeVbo_);
    g.BindVertexArray(probeVao_);
    g.BindBuffer(GL_ARRAY_BUFFER, probeVbo_);
    g.BufferData(GL_ARRAY_BUFFER, sizeof(probeVerts), probeVerts, GL_STATIC_DRAW);
    const GLsizei stride = static_cast<GLsizei>(12 * sizeof(float));
    g.EnableVertexAttribArray(0);
    g.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
    g.EnableVertexAttribArray(1);
    g.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(3 * sizeof(float)));
    g.EnableVertexAttribArray(2);
    g.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(6 * sizeof(float)));
    g.EnableVertexAttribArray(3);
    g.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(8 * sizeof(float)));
    g.EnableVertexAttribArray(4);
    g.VertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(11 * sizeof(float)));
    g.BindVertexArray(0);

    // Compile built-in default fallback shader
    double compileMs = 0.0;
    const std::string defaultFrag = composeFragmentShader(kDefaultSurfaceGlsl);
    if (!buildProgram(defaultFrag, defaultProgram_, compileMs, errorOut)) {
      return false;
    }
    lastCompileMs_ = compileMs;
    return true;
  }

  void ShaderManager::shutdown() {
    auto &g = render::gl();
    if (!g.loaded) {
      return;
    }
    for (auto &[id, handle] : programs_) {
      if (handle.programId != 0) {
        g.DeleteProgram(handle.programId);
      }
    }
    programs_.clear();
    lruOrder_.clear();

    if (defaultProgram_ != 0) {
      g.DeleteProgram(defaultProgram_);
      defaultProgram_ = 0;
    }
    if (probeVbo_ != 0) {
      g.DeleteBuffers(1, &probeVbo_);
      probeVbo_ = 0;
    }
    if (probeVao_ != 0) {
      g.DeleteVertexArrays(1, &probeVao_);
      probeVao_ = 0;
    }
    if (probeFbo_ != 0) {
      g.DeleteFramebuffers(1, &probeFbo_);
      probeFbo_ = 0;
    }
    if (probeTex_ != 0) {
      glDeleteTextures(1, &probeTex_);
      probeTex_ = 0;
    }
  }

  bool ShaderManager::compileStage(GLenum type, const std::string &source, GLuint &shaderOut,
                                   std::string &errorOut) {
    auto &g = render::gl();
    const GLuint shader = g.CreateShader(type);
    const char *srcPtr = source.c_str();
    g.ShaderSource(shader, 1, &srcPtr, nullptr);
    g.CompileShader(shader);

    GLint status = GL_FALSE;
    g.GetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
      GLint logLen = 0;
      g.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
      std::string log(static_cast<std::size_t>(std::max(logLen, 1)), '\0');
      g.GetShaderInfoLog(shader, logLen, nullptr, log.data());
      g.DeleteShader(shader);
      errorOut = "GLSL compile error: " + log;
      return false;
    }
    shaderOut = shader;
    return true;
  }

  bool ShaderManager::buildProgram(const std::string &fragmentSource, GLuint &programOut,
                                   double &compileMsOut, std::string &errorOut) {
    auto &g = render::gl();
    const auto t0 = std::chrono::steady_clock::now();

    GLuint vs = 0;
    GLuint fs = 0;
    if (!compileStage(GL_VERTEX_SHADER, kFixedVertexShader, vs, errorOut)) {
      return false;
    }
    if (!compileStage(GL_FRAGMENT_SHADER, fragmentSource, fs, errorOut)) {
      g.DeleteShader(vs);
      return false;
    }

    const GLuint prog = g.CreateProgram();
    g.AttachShader(prog, vs);
    g.AttachShader(prog, fs);
    g.LinkProgram(prog);

    GLint linked = GL_FALSE;
    g.GetProgramiv(prog, GL_LINK_STATUS, &linked);
    g.DetachShader(prog, vs);
    g.DetachShader(prog, fs);
    g.DeleteShader(vs);
    g.DeleteShader(fs);

    const auto t1 = std::chrono::steady_clock::now();
    compileMsOut = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (linked == GL_FALSE) {
      GLint logLen = 0;
      g.GetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
      std::string log(static_cast<std::size_t>(std::max(logLen, 1)), '\0');
      g.GetProgramInfoLog(prog, logLen, nullptr, log.data());
      g.DeleteProgram(prog);
      errorOut = "GLSL link error: " + log;
      return false;
    }

    programOut = prog;
    return true;
  }

  bool ShaderManager::runControlledProbe(GLuint program, double &probeMsOut,
                                         std::string &errorOut) {
    auto &g = render::gl();
    GLint prevViewport[4] = {0, 0, 1280, 720};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Clear pre-existing GL errors
    while (glGetError() != GL_NO_ERROR) {
    }

    const auto t0 = std::chrono::steady_clock::now();
    g.BindFramebuffer(GL_FRAMEBUFFER, probeFbo_);
    glViewport(0, 0, 16, 16);
    glDisable(GL_DEPTH_TEST);
    g.UseProgram(program);

    const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    const GLint locVp = g.GetUniformLocation(program, "u_viewProj");
    if (locVp >= 0) {
      g.UniformMatrix4fv(locVp, 1, GL_FALSE, identity);
    }
    const GLint locTime = g.GetUniformLocation(program, "u_time");
    if (locTime >= 0) {
      g.Uniform1f(locTime, 1.0F);
    }
    const GLint locLight = g.GetUniformLocation(program, "u_lightDir");
    if (locLight >= 0) {
      g.Uniform3f(locLight, 0.3F, 0.9F, 0.3F);
    }

    g.BindVertexArray(probeVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFinish();
    g.BindVertexArray(0);
    g.UseProgram(0);
    g.BindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    const auto t1 = std::chrono::steady_clock::now();
    probeMsOut = std::chrono::duration<double, std::milli>(t1 - t0).count();

    const GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      errorOut = "Controlled render probe failed with GL error: " + std::to_string(err);
      return false;
    }
    if (probeMsOut > 120.0) {
      errorOut = "Controlled render probe exceeded safety budget (" + std::to_string(probeMsOut) +
                 " ms)";
      return false;
    }
    return true;
  }

  void ShaderManager::evictIfNeeded() {
    auto &g = render::gl();
    while (programs_.size() >= maxActivePrograms_ && !lruOrder_.empty()) {
      const std::string victim = lruOrder_.back();
      lruOrder_.pop_back();
      auto it = programs_.find(victim);
      if (it != programs_.end()) {
        if (it->second.programId != 0) {
          g.DeleteProgram(it->second.programId);
        }
        programs_.erase(it);
      }
    }
  }

  ShaderCompileResult ShaderManager::compileAndActivate(const std::string &sectorId,
                                                        const std::string &surfaceGlsl,
                                                        const std::string &aiModel,
                                                        ShaderCache *cache,
                                                        const std::string &prompt,
                                                        double generationMs) {
    ShaderCompileResult result{};
    const auto validation = ShaderValidator::validate(surfaceGlsl);
    if (!validation.valid) {
      result.errorMessage = validation.errorMessage;
      lastError_ = result.errorMessage;
      return result;
    }

    const std::string fullFrag = composeFragmentShader(validation.sanitizedCode);
    GLuint newProg = 0;
    if (!buildProgram(fullFrag, newProg, result.compileMs, result.errorMessage)) {
      lastError_ = result.errorMessage;
      return result;
    }

    if (!runControlledProbe(newProg, result.probeMs, result.errorMessage)) {
      render::gl().DeleteProgram(newProg);
      lastError_ = result.errorMessage;
      return result;
    }

    // Hot reload: replace existing program for this sector safely
    auto existing = programs_.find(sectorId);
    if (existing != programs_.end()) {
      if (existing->second.programId != 0) {
        render::gl().DeleteProgram(existing->second.programId);
      }
      lruOrder_.remove(sectorId);
      programs_.erase(existing);
    } else {
      evictIfNeeded();
    }

    const std::string hash = ShaderCache::computeHash(validation.sanitizedCode);
    ShaderProgramHandle handle{};
    handle.programId = newProg;
    handle.sectorId = sectorId;
    handle.shaderHash = hash;
    handle.aiModel = aiModel;
    handle.compileMs = result.compileMs;
    handle.probeMs = result.probeMs;
    handle.isAI = true;

    programs_.emplace(sectorId, handle);
    lruOrder_.push_front(sectorId);

    lastCompileMs_ = result.compileMs;
    lastError_.clear();

    if (cache != nullptr) {
      CachedShaderRecord rec{};
      rec.sectorId = sectorId;
      rec.shaderHash = hash;
      rec.glslSource = validation.sanitizedCode;
      rec.aiModel = aiModel;
      rec.prompt = prompt;
      rec.shaderApiVersion = kShaderApiVersion;
      rec.generationMs = generationMs;
      rec.compileMs = result.compileMs;
      rec.valid = true;
      (void)cache->save(rec);
    }

    result.success = true;
    return result;
  }

  const ShaderProgramHandle *ShaderManager::findSectorProgram(const std::string &sectorId) {
    auto it = programs_.find(sectorId);
    if (it == programs_.end()) {
      return nullptr;
    }
    return &it->second;
  }

} // namespace ailivingmaze::shader
