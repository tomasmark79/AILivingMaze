#pragma once

#include "Render/Camera.hpp"
#include "Render/GLFunctions.hpp"
#include "Shader/MaterialManager.hpp"
#include "World/World.hpp"

#include <cstddef>
#include <unordered_map>

namespace ailivingmaze::render {

  struct RenderStats {
    std::size_t loadedChunks = 0;
    std::size_t renderedChunks = 0;
    std::size_t culledChunks = 0;
    std::size_t drawCalls = 0;
    std::size_t renderedTriangles = 0;
  };

  class Renderer {
  public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void shutdown();

    void syncChunks(const world::ChunkManager &chunkManager);
    RenderStats render(const world::World &world, const Camera &camera,
                       const shader::MaterialManager &materials, float aspectRatio,
                       float elapsedSeconds);

  private:
    struct GpuChunkMesh {
      GLuint vao = 0;
      GLuint vbo = 0;
      GLsizei vertexCount = 0;
      uint64_t version = 0;
    };

    void uploadChunk(const world::Chunk &chunk);
    void destroyMesh(GpuChunkMesh &mesh);

    std::unordered_map<world::ChunkCoord, GpuChunkMesh, world::ChunkCoordHash> gpuChunks_;
  };

} // namespace ailivingmaze::render
