#pragma once

#include "World/MazeGenerator.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace ailivingmaze::world {

  constexpr int kChunksPerSector = 2;

  struct SectorCoord {
    int32_t x = 0;
    int32_t z = 0;

    bool operator==(const SectorCoord &other) const noexcept = default;
  };

  struct SectorTheme {
    std::string sectorId;
    SectorCoord coord{};
    int archetypeIndex = 0;
    std::string styleName;
    std::string atmosphere;
    std::string materialProperties;
    glm::vec3 primaryColor{0.2F, 0.7F, 0.9F};
    glm::vec3 secondaryColor{0.08F, 0.14F, 0.22F};
    glm::vec3 accentColor{0.95F, 0.4F, 0.2F};
    glm::vec3 fogColor{0.03F, 0.05F, 0.08F};
    float fogDensity = 0.022F;
    float lightIntensity = 1.0F;
  };

  class SectorManager {
  public:
    explicit SectorManager(uint64_t worldSeed = 1337ULL) noexcept : worldSeed_(worldSeed) {}

    [[nodiscard]] static SectorCoord sectorForChunk(ChunkCoord chunk) noexcept;
    [[nodiscard]] static SectorCoord sectorForWorldPos(const glm::vec3 &pos) noexcept;
    [[nodiscard]] static std::string sectorIdFromCoord(SectorCoord coord);

    [[nodiscard]] SectorTheme describeSector(SectorCoord coord) const;
    [[nodiscard]] std::vector<SectorCoord> nearbySectors(SectorCoord center, int radius) const;

  private:
    uint64_t worldSeed_ = 1337ULL;
  };

} // namespace ailivingmaze::world
