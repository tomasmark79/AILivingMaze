#pragma once

#include "World/MazeGenerator.hpp"
#include "World/SectorManager.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace ailivingmaze::world {

  struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 localPos;
    float surfaceType; // 0.0 = floor, 1.0 = wall, 2.0 = ceiling
  };

  struct Chunk {
    ChunkCoord coord{};
    SectorCoord sectorCoord{};
    std::string sectorId;
    MazeChunkData maze{};
    std::vector<MeshVertex> vertices;
    glm::vec3 minBounds{};
    glm::vec3 maxBounds{};
    uint64_t meshVersion = 1;
  };

  class ChunkManager {
  public:
    ChunkManager(const MazeGenerator &generator, int loadRadius = 2, int maxLoadedChunks = 49);

    void updateAround(const glm::vec3 &playerPos);

    [[nodiscard]] static ChunkCoord chunkForWorldPos(const glm::vec3 &pos) noexcept;
    [[nodiscard]] const Chunk *findChunk(ChunkCoord coord) const noexcept;
    [[nodiscard]] const std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> &
        loadedChunks() const noexcept {
      return chunks_;
    }
    [[nodiscard]] std::size_t loadedCount() const noexcept { return chunks_.size(); }
    [[nodiscard]] int loadRadius() const noexcept { return loadRadius_; }
    void setLoadRadius(int radius) noexcept { loadRadius_ = radius; }

    // Builds geometry for a chunk (pure CPU, testable without OpenGL)
    static void buildChunkMesh(Chunk &chunk);

  private:
    const MazeGenerator &generator_;
    int loadRadius_ = 2;
    int maxLoadedChunks_ = 49;
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks_;
  };

} // namespace ailivingmaze::world
