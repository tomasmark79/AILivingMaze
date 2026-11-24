#include "World/MazeGenerator.hpp"

#include <algorithm>
#include <queue>
#include <stack>

namespace ailivingmaze::world {

  namespace {
    struct Rng {
      uint64_t state;
      uint64_t next() noexcept {
        state += 0x9e3779b97f4a7c15ULL;
        uint64_t z = state;
        z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31U);
      }
      int range(int n) noexcept { return static_cast<int>(next() % static_cast<uint64_t>(n)); }
    };
  } // namespace

  uint64_t MazeGenerator::splitmix64(uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31U);
  }

  uint64_t MazeGenerator::hashCoords(int32_t x, int32_t z, uint64_t salt) const noexcept {
    const uint64_t ux = static_cast<uint32_t>(x);
    const uint64_t uz = static_cast<uint32_t>(z);
    return splitmix64(worldSeed_ ^ splitmix64((ux << 32U) | uz) ^ salt);
  }

  int MazeGenerator::borderPortalIndex(ChunkCoord a, ChunkCoord b, int slot) const noexcept {
    // Canonical ordering so both adjacent chunks compute the exact same portal index
    if (b.x < a.x || (b.x == a.x && b.z < a.z)) {
      std::swap(a, b);
    }
    const uint64_t h1 = hashCoords(a.x, a.z, 0xA5A5A5A5ULL + static_cast<uint64_t>(slot) * 31ULL);
    const uint64_t h2 = hashCoords(b.x, b.z, 0x5A5A5A5AULL + static_cast<uint64_t>(slot) * 97ULL);
    return static_cast<int>(splitmix64(h1 ^ h2) % static_cast<uint64_t>(kChunkCells));
  }

  MazeChunkData MazeGenerator::generateChunk(ChunkCoord coord) const {
    MazeChunkData chunk{};
    chunk.coord = coord;
    chunk.cells.fill(WALL_ALL);

    Rng rng{hashCoords(coord.x, coord.z, 0x123456789ABCDEFULL)};

    std::array<bool, kChunkCells * kChunkCells> visited{};
    std::stack<std::pair<int, int>> stack;

    const int startX = rng.range(kChunkCells);
    const int startZ = rng.range(kChunkCells);
    visited[static_cast<std::size_t>(startZ * kChunkCells + startX)] = true;
    stack.emplace(startX, startZ);

    struct Dir {
      int dx;
      int dz;
      uint8_t bit;
      uint8_t opp;
    };
    constexpr std::array<Dir, 4> kDirs{{
        {0, -1, WALL_NORTH, WALL_SOUTH},
        {0, 1, WALL_SOUTH, WALL_NORTH},
        {1, 0, WALL_EAST, WALL_WEST},
        {-1, 0, WALL_WEST, WALL_EAST},
    }};

    // Randomized iterative DFS spanning tree -> guarantees 100% internal connectivity
    while (!stack.empty()) {
      auto [cx, cz] = stack.top();
      std::array<int, 4> candidates{};
      int count = 0;
      for (int i = 0; i < 4; ++i) {
        const int nx = cx + kDirs[i].dx;
        const int nz = cz + kDirs[i].dz;
        if (nx >= 0 && nx < kChunkCells && nz >= 0 && nz < kChunkCells &&
            !visited[static_cast<std::size_t>(nz * kChunkCells + nx)]) {
          candidates[count++] = i;
        }
      }
      if (count == 0) {
        stack.pop();
        continue;
      }

      const int pick = candidates[rng.range(count)];
      const auto &d = kDirs[pick];
      const int nx = cx + d.dx;
      const int nz = cz + d.dz;

      chunk.cellAt(cx, cz) &= static_cast<uint8_t>(~d.bit);
      chunk.cellAt(nx, nz) &= static_cast<uint8_t>(~d.opp);
      visited[static_cast<std::size_t>(nz * kChunkCells + nx)] = true;
      stack.emplace(nx, nz);
    }

    // Add braided loops (~16% of internal walls) for richer architectural exploration
    for (int z = 0; z < kChunkCells; ++z) {
      for (int x = 0; x < kChunkCells; ++x) {
        if (x + 1 < kChunkCells && (chunk.cellAt(x, z) & WALL_EAST) != 0 && rng.range(100) < 16) {
          chunk.cellAt(x, z) &= static_cast<uint8_t>(~WALL_EAST);
          chunk.cellAt(x + 1, z) &= static_cast<uint8_t>(~WALL_WEST);
        }
        if (z + 1 < kChunkCells && (chunk.cellAt(x, z) & WALL_SOUTH) != 0 && rng.range(100) < 16) {
          chunk.cellAt(x, z) &= static_cast<uint8_t>(~WALL_SOUTH);
          chunk.cellAt(x, z + 1) &= static_cast<uint8_t>(~WALL_NORTH);
        }
      }
    }

    // Deterministic border portals connecting adjacent chunks (2 portals per edge)
    for (int slot = 0; slot < 2; ++slot) {
      const int northX = borderPortalIndex(coord, {coord.x, coord.z - 1}, slot);
      chunk.cellAt(northX, 0) &= static_cast<uint8_t>(~WALL_NORTH);

      const int southX = borderPortalIndex(coord, {coord.x, coord.z + 1}, slot);
      chunk.cellAt(southX, kChunkCells - 1) &= static_cast<uint8_t>(~WALL_SOUTH);

      const int westZ = borderPortalIndex(coord, {coord.x - 1, coord.z}, slot);
      chunk.cellAt(0, westZ) &= static_cast<uint8_t>(~WALL_WEST);

      const int eastZ = borderPortalIndex(coord, {coord.x + 1, coord.z}, slot);
      chunk.cellAt(kChunkCells - 1, eastZ) &= static_cast<uint8_t>(~WALL_EAST);
    }

    return chunk;
  }

  bool MazeGenerator::verifyChunkConnectivity(const MazeChunkData &chunk) {
    std::array<bool, kChunkCells * kChunkCells> visited{};
    std::queue<std::pair<int, int>> q;
    visited[0] = true;
    q.emplace(0, 0);
    int reached = 0;

    while (!q.empty()) {
      auto [x, z] = q.front();
      q.pop();
      ++reached;
      const uint8_t mask = chunk.cellAt(x, z);

      auto tryVisit = [&](int nx, int nz, uint8_t wallBit) {
        if ((mask & wallBit) == 0 && nx >= 0 && nx < kChunkCells && nz >= 0 && nz < kChunkCells) {
          const auto idx = static_cast<std::size_t>(nz * kChunkCells + nx);
          if (!visited[idx]) {
            visited[idx] = true;
            q.emplace(nx, nz);
          }
        }
      };

      tryVisit(x, z - 1, WALL_NORTH);
      tryVisit(x, z + 1, WALL_SOUTH);
      tryVisit(x + 1, z, WALL_EAST);
      tryVisit(x - 1, z, WALL_WEST);
    }

    if (reached != kChunkCells * kChunkCells) {
      return false;
    }

    bool hasNorth = false;
    bool hasSouth = false;
    bool hasWest = false;
    bool hasEast = false;
    for (int i = 0; i < kChunkCells; ++i) {
      if ((chunk.cellAt(i, 0) & WALL_NORTH) == 0) {
        hasNorth = true;
      }
      if ((chunk.cellAt(i, kChunkCells - 1) & WALL_SOUTH) == 0) {
        hasSouth = true;
      }
      if ((chunk.cellAt(0, i) & WALL_WEST) == 0) {
        hasWest = true;
      }
      if ((chunk.cellAt(kChunkCells - 1, i) & WALL_EAST) == 0) {
        hasEast = true;
      }
    }
    return hasNorth && hasSouth && hasWest && hasEast;
  }

} // namespace ailivingmaze::world
