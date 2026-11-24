#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace ailivingmaze::shader {

  struct CachedShaderRecord {
    std::string sectorId;
    std::string shaderHash;
    std::string glslSource;
    std::string aiModel;
    std::string prompt;
    std::string shaderApiVersion;
    std::string createdAtUtc;
    double generationMs = 0.0;
    double compileMs = 0.0;
    bool valid = true;
  };

  struct ShaderCacheStats {
    std::size_t hits = 0;
    std::size_t misses = 0;
    std::size_t storedEntries = 0;
  };

  class ShaderCache {
  public:
    explicit ShaderCache(std::filesystem::path cacheDir);

    [[nodiscard]] bool save(const CachedShaderRecord &record);
    [[nodiscard]] std::optional<CachedShaderRecord> loadLatestForSector(const std::string &sectorId);

    bool invalidateSector(const std::string &sectorId);
    bool invalidateAll();

    [[nodiscard]] static std::string computeHash(const std::string &glslSource);
    [[nodiscard]] const std::filesystem::path &directory() const noexcept { return cacheDir_; }
    [[nodiscard]] ShaderCacheStats stats() const noexcept { return stats_; }

  private:
    static std::string sanitizeId(const std::string &id);
    std::size_t countEntriesOnDisk() const;

    std::filesystem::path cacheDir_;
    ShaderCacheStats stats_{};
  };

} // namespace ailivingmaze::shader
