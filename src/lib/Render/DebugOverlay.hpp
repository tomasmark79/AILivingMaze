#pragma once

#include "Render/GLFunctions.hpp"

#include <cstddef>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace ailivingmaze::render {

  struct DebugOverlayData {
    float fps = 0.0F;
    float frameTimeMs = 0.0F;
    glm::vec3 playerPos{0.0F};
    int chunkX = 0;
    int chunkZ = 0;
    std::string sectorId;
    std::string sectorStyle;
    std::size_t loadedChunks = 0;
    std::size_t renderedChunks = 0;
    std::size_t drawCalls = 0;
    std::size_t activeShaders = 0;
    bool usingAIShaders = true;
    bool cursorCaptured = true;
    bool invertMouseY = false;
    std::string aiProviderStatus;
    std::size_t pendingAIRequests = 0;
    double lastGenerationMs = 0.0;
    double lastCompileMs = 0.0;
    std::size_t cacheHits = 0;
    std::size_t cacheMisses = 0;
    std::size_t cacheStored = 0;
    std::string lastShaderError;
  };

  class DebugOverlay {
  public:
    DebugOverlay() = default;
    ~DebugOverlay();

    DebugOverlay(const DebugOverlay &) = delete;
    DebugOverlay &operator=(const DebugOverlay &) = delete;

    bool init(std::string &errorOut);
    void shutdown();

    void setVisible(bool visible) noexcept { visible_ = visible; }
    void toggleVisible() noexcept { visible_ = !visible_; }
    [[nodiscard]] bool isVisible() const noexcept { return visible_; }

    void render(int screenWidth, int screenHeight, const DebugOverlayData &data);

  private:
    struct QuadVertex {
      float x;
      float y;
      float u;
      float v;
      float r;
      float g;
      float b;
      float a;
      float textured;
    };

    void appendRect(std::vector<QuadVertex> &verts, float x, float y, float w, float h,
                    const glm::vec4 &color);
    void appendText(std::vector<QuadVertex> &verts, float x, float y, float scale,
                    const std::string &text, const glm::vec4 &color);

    bool visible_ = true;
    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint fontTex_ = 0;
  };

} // namespace ailivingmaze::render
