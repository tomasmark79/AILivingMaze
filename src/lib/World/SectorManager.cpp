#include "World/SectorManager.hpp"

#include <array>
#include <cmath>

namespace ailivingmaze::world {

  namespace {
    struct ArchetypePreset {
      const char *styleName;
      const char *atmosphere;
      const char *materialProperties;
      glm::vec3 primary;
      glm::vec3 secondary;
      glm::vec3 accent;
      glm::vec3 fog;
      float fogDensity;
    };

    constexpr std::array<ArchetypePreset, 9> kArchetypes{{
        {"Futuristic Tech Corridors",
         "Clean neon-lit cybernetic facility with subtle humming energy conduits",
         "Brushed titanium panels, recessed cyan circuit traces, matte anti-slip deck",
         {0.15F, 0.78F, 0.95F},
         {0.07F, 0.11F, 0.16F},
         {0.05F, 0.95F, 0.85F},
         {0.02F, 0.05F, 0.09F},
         0.020F},
        {"Organic Pulsing Walls",
         "Bioluminescent living labyrinth with slow rhythmic breathing veins",
         "Translucent chitinous membrane, glowing amber-magenta veins, damp floor",
         {0.78F, 0.22F, 0.42F},
         {0.14F, 0.06F, 0.10F},
         {0.98F, 0.62F, 0.18F},
         {0.07F, 0.02F, 0.05F},
         0.028F},
        {"Crystalline Structures",
         "Prismatic cathedral of faceted quartz and refractive ice corridors",
         "Sharp Voronoi crystal facets, high specular sheen, violet-cyan iridescence",
         {0.58F, 0.48F, 0.98F},
         {0.10F, 0.14F, 0.28F},
         {0.45F, 0.95F, 0.98F},
         {0.04F, 0.05F, 0.12F},
         0.018F},
        {"Dark Industrial Vault",
         "Heavy subterranean foundry with furnace glow and riveted steel",
         "Weathered dark iron plates, hazard stripes, warm ember emissive grates",
         {0.42F, 0.36F, 0.32F},
         {0.10F, 0.09F, 0.08F},
         {0.98F, 0.44F, 0.08F},
         {0.05F, 0.04F, 0.03F},
         0.030F},
        {"Abstract Mathematical Patterns",
         "Non-Euclidean geometric sanctum driven by interference waves and fractals",
         "High-contrast procedural grids, golden-ratio harmonics, luminous equations",
         {0.92F, 0.82F, 0.35F},
         {0.06F, 0.08F, 0.14F},
         {0.25F, 0.88F, 0.96F},
         {0.03F, 0.04F, 0.07F},
         0.019F},
        {"Alien Xenomaterials",
         "Ancient extraterrestrial monoliths with shifting iridescent glyphs",
         "Obsidian hexagonal scales, emerald plasma seams, resonant monolith stone",
         {0.18F, 0.86F, 0.46F},
         {0.05F, 0.12F, 0.09F},
         {0.68F, 0.98F, 0.25F},
         {0.02F, 0.07F, 0.05F},
         0.024F},
        {"Energy Lattice Structures",
         "High-voltage containment maze radiating plasma arcs and standing waves",
         "Dark carbon conduits, intense electric blue and magenta force fields",
         {0.22F, 0.45F, 0.98F},
         {0.04F, 0.05F, 0.12F},
         {0.95F, 0.25F, 0.85F},
         {0.03F, 0.03F, 0.10F},
         0.022F},
        {"Surreal Dreamscape",
         "Escher-like shifting halls with warm twilight gradients and floating ripples",
         "Iridescent porcelain tiles, impossible optical stripes, pastel aurora glow",
         {0.92F, 0.52F, 0.72F},
         {0.16F, 0.12F, 0.24F},
         {0.98F, 0.84F, 0.42F},
         {0.08F, 0.05F, 0.11F},
         0.021F},
        {"Minimalist Monumental Halls",
         "Brutalist alabaster and basalt gallery with crisp architectural light slits",
         "Smooth cast concrete, subtle travertine grain, warm indirect linear light",
         {0.82F, 0.80F, 0.76F},
         {0.18F, 0.18F, 0.20F},
         {0.98F, 0.88F, 0.68F},
         {0.06F, 0.06F, 0.07F},
         0.016F},
    }};

    int32_t floorDiv(int32_t a, int32_t b) noexcept {
      int32_t q = a / b;
      int32_t r = a % b;
      if (r != 0 && ((r < 0) != (b < 0))) {
        --q;
      }
      return q;
    }

    uint64_t hashSector(uint64_t seed, SectorCoord c) noexcept {
      uint64_t x = seed ^ (static_cast<uint64_t>(static_cast<uint32_t>(c.x)) << 32U) ^
                   static_cast<uint64_t>(static_cast<uint32_t>(c.z));
      x += 0x9e3779b97f4a7c15ULL;
      x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
      x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
      return x ^ (x >> 31U);
    }
  } // namespace

  SectorCoord SectorManager::sectorForChunk(ChunkCoord chunk) noexcept {
    return {floorDiv(chunk.x, kChunksPerSector), floorDiv(chunk.z, kChunksPerSector)};
  }

  SectorCoord SectorManager::sectorForWorldPos(const glm::vec3 &pos) noexcept {
    const int32_t cx = static_cast<int32_t>(std::floor(pos.x / kChunkWorldSize));
    const int32_t cz = static_cast<int32_t>(std::floor(pos.z / kChunkWorldSize));
    return sectorForChunk({cx, cz});
  }

  std::string SectorManager::sectorIdFromCoord(SectorCoord coord) {
    return "sector_" + std::to_string(coord.x) + "_" + std::to_string(coord.z);
  }

  SectorTheme SectorManager::describeSector(SectorCoord coord) const {
    const uint64_t h = hashSector(worldSeed_, coord);
    const int idx = static_cast<int>(h % kArchetypes.size());
    const auto &p = kArchetypes[static_cast<std::size_t>(idx)];

    SectorTheme theme;
    theme.sectorId = sectorIdFromCoord(coord);
    theme.coord = coord;
    theme.archetypeIndex = idx;
    theme.styleName = p.styleName;
    theme.atmosphere = p.atmosphere;
    theme.materialProperties = p.materialProperties;
    theme.primaryColor = p.primary;
    theme.secondaryColor = p.secondary;
    theme.accentColor = p.accent;
    theme.fogColor = p.fog;
    theme.fogDensity = p.fogDensity;
    theme.lightIntensity = 1.0F;
    return theme;
  }

  std::vector<SectorCoord> SectorManager::nearbySectors(SectorCoord center, int radius) const {
    std::vector<SectorCoord> result;
    for (int dz = -radius; dz <= radius; ++dz) {
      for (int dx = -radius; dx <= radius; ++dx) {
        result.push_back({center.x + dx, center.z + dz});
      }
    }
    return result;
  }

} // namespace ailivingmaze::world
