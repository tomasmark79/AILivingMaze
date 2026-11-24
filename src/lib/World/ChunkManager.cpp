#include "World/ChunkManager.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ailivingmaze::world {

  namespace {
    void appendQuad(std::vector<MeshVertex> &verts, const glm::vec3 &p0, const glm::vec3 &p1,
                    const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec3 &normal,
                    const glm::vec2 &uvScale, const glm::vec3 &chunkOrigin, float surfaceType) {
      auto makeV = [&](const glm::vec3 &p, const glm::vec2 &uv) {
        return MeshVertex{p, normal, uv * uvScale, p - chunkOrigin, surfaceType};
      };
      const MeshVertex v0 = makeV(p0, {0.0F, 0.0F});
      const MeshVertex v1 = makeV(p1, {1.0F, 0.0F});
      const MeshVertex v2 = makeV(p2, {1.0F, 1.0F});
      const MeshVertex v3 = makeV(p3, {0.0F, 1.0F});
      verts.push_back(v0);
      verts.push_back(v1);
      verts.push_back(v2);
      verts.push_back(v0);
      verts.push_back(v2);
      verts.push_back(v3);
    }

    void appendBoxWalls(std::vector<MeshVertex> &verts, const glm::vec3 &minP,
                        const glm::vec3 &maxP, const glm::vec3 &chunkOrigin) {
      const float sx = maxP.x - minP.x;
      const float sy = maxP.y - minP.y;
      const float sz = maxP.z - minP.z;

      // +Z face
      appendQuad(verts, {minP.x, minP.y, maxP.z}, {maxP.x, minP.y, maxP.z}, {maxP.x, maxP.y, maxP.z},
                 {minP.x, maxP.y, maxP.z}, {0.0F, 0.0F, 1.0F}, {sx / kCellSize, sy / kWallHeight},
                 chunkOrigin, 1.0F);
      // -Z face
      appendQuad(verts, {maxP.x, minP.y, minP.z}, {minP.x, minP.y, minP.z}, {minP.x, maxP.y, minP.z},
                 {maxP.x, maxP.y, minP.z}, {0.0F, 0.0F, -1.0F}, {sx / kCellSize, sy / kWallHeight},
                 chunkOrigin, 1.0F);
      // +X face
      appendQuad(verts, {maxP.x, minP.y, maxP.z}, {maxP.x, minP.y, minP.z}, {maxP.x, maxP.y, minP.z},
                 {maxP.x, maxP.y, maxP.z}, {1.0F, 0.0F, 0.0F}, {sz / kCellSize, sy / kWallHeight},
                 chunkOrigin, 1.0F);
      // -X face
      appendQuad(verts, {minP.x, minP.y, minP.z}, {minP.x, minP.y, maxP.z}, {minP.x, maxP.y, maxP.z},
                 {minP.x, maxP.y, minP.z}, {-1.0F, 0.0F, 0.0F}, {sz / kCellSize, sy / kWallHeight},
                 chunkOrigin, 1.0F);
    }
  } // namespace

  ChunkManager::ChunkManager(const MazeGenerator &generator, int loadRadius, int maxLoadedChunks)
      : generator_(generator), loadRadius_(loadRadius), maxLoadedChunks_(maxLoadedChunks) {}

  ChunkCoord ChunkManager::chunkForWorldPos(const glm::vec3 &pos) noexcept {
    return {static_cast<int32_t>(std::floor(pos.x / kChunkWorldSize)),
            static_cast<int32_t>(std::floor(pos.z / kChunkWorldSize))};
  }

  const Chunk *ChunkManager::findChunk(ChunkCoord coord) const noexcept {
    const auto it = chunks_.find(coord);
    return it != chunks_.end() ? &it->second : nullptr;
  }

  void ChunkManager::updateAround(const glm::vec3 &playerPos) {
    const ChunkCoord center = chunkForWorldPos(playerPos);

    // Load missing chunks within radius
    for (int dz = -loadRadius_; dz <= loadRadius_; ++dz) {
      for (int dx = -loadRadius_; dx <= loadRadius_; ++dx) {
        const ChunkCoord coord{center.x + dx, center.z + dz};
        if (chunks_.find(coord) == chunks_.end()) {
          Chunk chunk{};
          chunk.coord = coord;
          chunk.sectorCoord = SectorManager::sectorForChunk(coord);
          chunk.sectorId = SectorManager::sectorIdFromCoord(chunk.sectorCoord);
          chunk.maze = generator_.generateChunk(coord);
          buildChunkMesh(chunk);
          chunks_.emplace(coord, std::move(chunk));
        }
      }
    }

    // Unload chunks beyond radius + 1 hysteresis
    const int unloadRadius = loadRadius_ + 1;
    for (auto it = chunks_.begin(); it != chunks_.end();) {
      const int dx = std::abs(it->first.x - center.x);
      const int dz = std::abs(it->first.z - center.z);
      if (dx > unloadRadius || dz > unloadRadius) {
        it = chunks_.erase(it);
      } else {
        ++it;
      }
    }

    // Enforce hard cap by evicting farthest chunks
    while (static_cast<int>(chunks_.size()) > maxLoadedChunks_) {
      auto farthest = chunks_.begin();
      int maxDistSq = -1;
      for (auto it = chunks_.begin(); it != chunks_.end(); ++it) {
        const int dx = it->first.x - center.x;
        const int dz = it->first.z - center.z;
        const int d2 = dx * dx + dz * dz;
        if (d2 > maxDistSq) {
          maxDistSq = d2;
          farthest = it;
        }
      }
      chunks_.erase(farthest);
    }
  }

  void ChunkManager::buildChunkMesh(Chunk &chunk) {
    chunk.vertices.clear();
    chunk.vertices.reserve(4096);

    const glm::vec3 origin{static_cast<float>(chunk.coord.x) * kChunkWorldSize, 0.0F,
                           static_cast<float>(chunk.coord.z) * kChunkWorldSize};
    chunk.minBounds = origin;
    chunk.maxBounds = origin + glm::vec3{kChunkWorldSize, kWallHeight, kChunkWorldSize};

    // Single floor quad and single ceiling quad for the whole chunk (tiled UVs)
    const glm::vec3 f0{origin.x, 0.0F, origin.z};
    const glm::vec3 f1{origin.x + kChunkWorldSize, 0.0F, origin.z};
    const glm::vec3 f2{origin.x + kChunkWorldSize, 0.0F, origin.z + kChunkWorldSize};
    const glm::vec3 f3{origin.x, 0.0F, origin.z + kChunkWorldSize};
    appendQuad(chunk.vertices, f0, f3, f2, f1, {0.0F, 1.0F, 0.0F},
               {static_cast<float>(kChunkCells), static_cast<float>(kChunkCells)}, origin, 0.0F);

    const glm::vec3 c0{origin.x, kWallHeight, origin.z};
    const glm::vec3 c1{origin.x + kChunkWorldSize, kWallHeight, origin.z};
    const glm::vec3 c2{origin.x + kChunkWorldSize, kWallHeight, origin.z + kChunkWorldSize};
    const glm::vec3 c3{origin.x, kWallHeight, origin.z + kChunkWorldSize};
    appendQuad(chunk.vertices, c0, c1, c2, c3, {0.0F, -1.0F, 0.0F},
               {static_cast<float>(kChunkCells), static_cast<float>(kChunkCells)}, origin, 2.0F);

    const float halfT = kWallThickness * 0.5F;

    for (int lz = 0; lz < kChunkCells; ++lz) {
      for (int lx = 0; lx < kChunkCells; ++lx) {
        const uint8_t mask = chunk.maze.cellAt(lx, lz);
        const float x0 = origin.x + static_cast<float>(lx) * kCellSize;
        const float z0 = origin.z + static_cast<float>(lz) * kCellSize;
        const float x1 = x0 + kCellSize;
        const float z1 = z0 + kCellSize;

        // Avoid duplicate internal walls: emit North for all cells, South only at lz == kChunkCells-1
        if ((mask & WALL_NORTH) != 0) {
          appendBoxWalls(chunk.vertices, {x0 - halfT, 0.0F, z0 - halfT},
                         {x1 + halfT, kWallHeight, z0 + halfT}, origin);
        }
        if (lz == kChunkCells - 1 && (mask & WALL_SOUTH) != 0) {
          appendBoxWalls(chunk.vertices, {x0 - halfT, 0.0F, z1 - halfT},
                         {x1 + halfT, kWallHeight, z1 + halfT}, origin);
        }
        // Emit West for all cells, East only at lx == kChunkCells-1
        if ((mask & WALL_WEST) != 0) {
          appendBoxWalls(chunk.vertices, {x0 - halfT, 0.0F, z0 - halfT},
                         {x0 + halfT, kWallHeight, z1 + halfT}, origin);
        }
        if (lx == kChunkCells - 1 && (mask & WALL_EAST) != 0) {
          appendBoxWalls(chunk.vertices, {x1 - halfT, 0.0F, z0 - halfT},
                         {x1 + halfT, kWallHeight, z1 + halfT}, origin);
        }
      }
    }
  }

} // namespace ailivingmaze::world
