#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace ailivingmaze::world {

  constexpr int kChunkCells = 8;
  constexpr float kCellSize = 4.0F;
  constexpr float kWallHeight = 4.2F;
  constexpr float kWallThickness = 0.45F;
  constexpr float kChunkWorldSize = static_cast<float>(kChunkCells) * kCellSize;

  enum WallMask : uint8_t {
    WALL_NONE = 0,
    WALL_NORTH = 1 << 0, // -Z
    WALL_SOUTH = 1 << 1, // +Z
    WALL_EAST = 1 << 2,  // +X
    WALL_WEST = 1 << 3,  // -X
    WALL_ALL = WALL_NORTH | WALL_SOUTH | WALL_EAST | WALL_WEST,
  };

  struct ChunkCoord {
    int32_t x = 0;
    int32_t z = 0;

    bool operator==(const ChunkCoord &other) const noexcept = default;
  };

  struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord &c) const noexcept {
      const uint64_t ux = static_cast<uint32_t>(c.x);
      const uint64_t uz = static_cast<uint32_t>(c.z);
      uint64_t h = (ux << 32U) ^ uz;
      h ^= (h >> 33U);
      h *= 0xff51afd7ed558ccdULL;
      h ^= (h >> 33U);
      return static_cast<std::size_t>(h);
    }
  };

  struct MazeChunkData {
    ChunkCoord coord{};
    std::array<uint8_t, kChunkCells * kChunkCells> cells{};

    [[nodiscard]] uint8_t cellAt(int lx, int lz) const noexcept {
      return cells[static_cast<std::size_t>(lz * kChunkCells + lx)];
    }

    uint8_t &cellAt(int lx, int lz) noexcept {
      return cells[static_cast<std::size_t>(lz * kChunkCells + lx)];
    }
  };

  class MazeGenerator {
  public:
    explicit MazeGenerator(uint64_t worldSeed = 1337ULL) noexcept : worldSeed_(worldSeed) {}

    [[nodiscard]] uint64_t seed() const noexcept { return worldSeed_; }
    [[nodiscard]] MazeChunkData generateChunk(ChunkCoord coord) const;

    // Verifies that every cell in the chunk is reachable from cell (0,0)
    // and that at least one portal exists on every chunk border.
    [[nodiscard]] static bool verifyChunkConnectivity(const MazeChunkData &chunk);

    // Deterministic border portal indices shared by two adjacent chunks.
    [[nodiscard]] int borderPortalIndex(ChunkCoord a, ChunkCoord b, int slot) const noexcept;

  private:
    static uint64_t splitmix64(uint64_t x) noexcept;
    [[nodiscard]] uint64_t hashCoords(int32_t x, int32_t z, uint64_t salt) const noexcept;

    uint64_t worldSeed_ = 1337ULL;
  };

} // namespace ailivingmaze::world
