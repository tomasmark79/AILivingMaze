#pragma once

#include "World/ChunkManager.hpp"
#include "World/MazeGenerator.hpp"
#include "World/SectorManager.hpp"

#include <cstdint>
#include <glm/glm.hpp>

namespace ailivingmaze::world {

  class World {
  public:
    explicit World(uint64_t seed = 1337ULL, int chunkLoadRadius = 2);

    void update(const glm::vec3 &playerPos);

    [[nodiscard]] uint64_t seed() const noexcept { return generator_.seed(); }
    [[nodiscard]] const MazeGenerator &mazeGenerator() const noexcept { return generator_; }
    [[nodiscard]] const ChunkManager &chunkManager() const noexcept { return chunkManager_; }
    [[nodiscard]] const SectorManager &sectorManager() const noexcept { return sectorManager_; }

    // Returns wall bitmask for the cell at global cell coordinates (gx, gz).
    // If chunk is not loaded, returns WALL_ALL (safe conservative solid boundary).
    [[nodiscard]] uint8_t cellWallsAtGlobal(int32_t gx, int32_t gz) const noexcept;

    // Safe starting spawn position (center of cell (0,0) in chunk (0,0))
    [[nodiscard]] static glm::vec3 defaultSpawnPosition() noexcept {
      return {kCellSize * 0.5F, 1.7F, kCellSize * 0.5F};
    }

  private:
    MazeGenerator generator_;
    SectorManager sectorManager_;
    ChunkManager chunkManager_;
  };

} // namespace ailivingmaze::world
