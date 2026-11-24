#include "World/World.hpp"

namespace ailivingmaze::world {

  namespace {
    int32_t floorDiv(int32_t a, int32_t b) noexcept {
      int32_t q = a / b;
      int32_t r = a % b;
      if (r != 0 && ((r < 0) != (b < 0))) {
        --q;
      }
      return q;
    }

    int32_t floorMod(int32_t a, int32_t b) noexcept {
      int32_t r = a % b;
      if (r < 0) {
        r += b;
      }
      return r;
    }
  } // namespace

  World::World(uint64_t seed, int chunkLoadRadius)
      : generator_(seed), sectorManager_(seed), chunkManager_(generator_, chunkLoadRadius) {}

  void World::update(const glm::vec3 &playerPos) { chunkManager_.updateAround(playerPos); }

  uint8_t World::cellWallsAtGlobal(int32_t gx, int32_t gz) const noexcept {
    const ChunkCoord c{floorDiv(gx, kChunkCells), floorDiv(gz, kChunkCells)};
    const Chunk *chunk = chunkManager_.findChunk(c);
    if (!chunk) {
      return WALL_ALL;
    }
    const int lx = floorMod(gx, kChunkCells);
    const int lz = floorMod(gz, kChunkCells);
    return chunk->maze.cellAt(lx, lz);
  }

} // namespace ailivingmaze::world
