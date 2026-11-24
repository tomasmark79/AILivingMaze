#include "Shader/ShaderCache.hpp"

#include "Shader/ShaderAPI.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace ailivingmaze::shader {

  namespace {
    std::string currentUtcIso8601() {
      const auto now = std::chrono::system_clock::now();
      const std::time_t tt = std::chrono::system_clock::to_time_t(now);
      std::tm utcTm{};
      gmtime_r(&tt, &utcTm);
      std::ostringstream oss;
      oss << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%SZ");
      return oss.str();
    }
  } // namespace

  ShaderCache::ShaderCache(std::filesystem::path cacheDir) : cacheDir_(std::move(cacheDir)) {
    std::error_code ec;
    std::filesystem::create_directories(cacheDir_, ec);
    stats_.storedEntries = countEntriesOnDisk();
  }

  std::string ShaderCache::sanitizeId(const std::string &id) {
    std::string out;
    out.reserve(id.size());
    for (char c : id) {
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
          c == '_' || c == '-') {
        out.push_back(c);
      } else {
        out.push_back('_');
      }
    }
    return out;
  }

  std::string ShaderCache::computeHash(const std::string &glslSource) {
    // 128-bit FNV-1a-like double hash formatted as 32 hex chars (deterministic, zero external deps)
    uint64_t h1 = 14695981039346656037ULL;
    uint64_t h2 = 1099511628211ULL ^ 0x9e3779b97f4a7c15ULL;
    for (unsigned char c : glslSource) {
      h1 ^= c;
      h1 *= 1099511628211ULL;
      h2 ^= static_cast<uint64_t>(c) + 0x517cc1b727220a95ULL;
      h2 *= 1099511628211ULL;
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << h1 << std::setw(16) << h2;
    return oss.str();
  }

  std::size_t ShaderCache::countEntriesOnDisk() const {
    std::error_code ec;
    if (!std::filesystem::exists(cacheDir_, ec)) {
      return 0;
    }
    std::size_t count = 0;
    for (const auto &entry : std::filesystem::directory_iterator(cacheDir_, ec)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        ++count;
      }
    }
    return count;
  }

  bool ShaderCache::save(const CachedShaderRecord &record) {
    std::error_code ec;
    std::filesystem::create_directories(cacheDir_, ec);
    if (ec) {
      return false;
    }

    const std::string safeSector = sanitizeId(record.sectorId);
    const std::string hash =
        record.shaderHash.empty() ? computeHash(record.glslSource) : record.shaderHash;
    const std::string baseName = safeSector + "_" + hash.substr(0, 12);

    const auto glslPath = cacheDir_ / (baseName + ".glsl");
    const auto metaPath = cacheDir_ / (baseName + ".json");
    const auto latestPath = cacheDir_ / (safeSector + ".latest");

    {
      std::ofstream glslOut(glslPath, std::ios::binary | std::ios::trunc);
      if (!glslOut) {
        return false;
      }
      glslOut << record.glslSource;
    }

    nlohmann::json meta;
    meta["sectorId"] = record.sectorId;
    meta["shaderHash"] = hash;
    meta["glslFile"] = glslPath.filename().string();
    meta["aiModel"] = record.aiModel;
    meta["prompt"] = record.prompt;
    meta["shaderApiVersion"] =
        record.shaderApiVersion.empty() ? kShaderApiVersion : record.shaderApiVersion;
    meta["createdAtUtc"] = record.createdAtUtc.empty() ? currentUtcIso8601() : record.createdAtUtc;
    meta["generationMs"] = record.generationMs;
    meta["compileMs"] = record.compileMs;
    meta["valid"] = record.valid;

    {
      std::ofstream metaOut(metaPath, std::ios::binary | std::ios::trunc);
      if (!metaOut) {
        return false;
      }
      metaOut << meta.dump(2);
    }

    {
      std::ofstream latestOut(latestPath, std::ios::binary | std::ios::trunc);
      if (!latestOut) {
        return false;
      }
      latestOut << metaPath.filename().string();
    }

    stats_.storedEntries = countEntriesOnDisk();
    return true;
  }

  std::optional<CachedShaderRecord> ShaderCache::loadLatestForSector(const std::string &sectorId) {
    const std::string safeSector = sanitizeId(sectorId);
    const auto latestPath = cacheDir_ / (safeSector + ".latest");

    std::error_code ec;
    if (!std::filesystem::exists(latestPath, ec)) {
      ++stats_.misses;
      return std::nullopt;
    }

    std::string metaFileName;
    {
      std::ifstream latestIn(latestPath);
      if (!latestIn || !std::getline(latestIn, metaFileName)) {
        ++stats_.misses;
        return std::nullopt;
      }
    }

    const auto metaPath = cacheDir_ / metaFileName;
    if (!std::filesystem::exists(metaPath, ec)) {
      ++stats_.misses;
      return std::nullopt;
    }

    try {
      std::ifstream metaIn(metaPath);
      nlohmann::json meta = nlohmann::json::parse(metaIn);

      if (!meta.value("valid", false)) {
        ++stats_.misses;
        return std::nullopt;
      }

      // Versioned cache check: invalidate if API version differs
      if (meta.value("shaderApiVersion", std::string{}) != kShaderApiVersion) {
        ++stats_.misses;
        return std::nullopt;
      }

      const std::string glslFile = meta.value("glslFile", std::string{});
      const auto glslPath = cacheDir_ / glslFile;
      std::ifstream glslIn(glslPath, std::ios::binary);
      if (!glslIn) {
        ++stats_.misses;
        return std::nullopt;
      }
      std::ostringstream ss;
      ss << glslIn.rdbuf();

      CachedShaderRecord rec;
      rec.sectorId = meta.value("sectorId", sectorId);
      rec.shaderHash = meta.value("shaderHash", std::string{});
      rec.glslSource = ss.str();
      rec.aiModel = meta.value("aiModel", std::string{});
      rec.prompt = meta.value("prompt", std::string{});
      rec.shaderApiVersion = meta.value("shaderApiVersion", std::string{});
      rec.createdAtUtc = meta.value("createdAtUtc", std::string{});
      rec.generationMs = meta.value("generationMs", 0.0);
      rec.compileMs = meta.value("compileMs", 0.0);
      rec.valid = true;

      // Verify content integrity against recorded hash
      if (!rec.shaderHash.empty() && computeHash(rec.glslSource) != rec.shaderHash) {
        ++stats_.misses;
        return std::nullopt;
      }

      ++stats_.hits;
      return rec;
    } catch (...) {
      ++stats_.misses;
      return std::nullopt;
    }
  }

  bool ShaderCache::invalidateSector(const std::string &sectorId) {
    const std::string safeSector = sanitizeId(sectorId);
    std::error_code ec;
    bool removedAny = false;
    if (!std::filesystem::exists(cacheDir_, ec)) {
      return false;
    }
    for (const auto &entry : std::filesystem::directory_iterator(cacheDir_, ec)) {
      const std::string fname = entry.path().filename().string();
      if (fname == safeSector + ".latest" || fname.rfind(safeSector + "_", 0) == 0) {
        std::filesystem::remove(entry.path(), ec);
        removedAny = true;
      }
    }
    stats_.storedEntries = countEntriesOnDisk();
    return removedAny;
  }

  bool ShaderCache::invalidateAll() {
    std::error_code ec;
    if (!std::filesystem::exists(cacheDir_, ec)) {
      return true;
    }
    for (const auto &entry : std::filesystem::directory_iterator(cacheDir_, ec)) {
      std::filesystem::remove_all(entry.path(), ec);
    }
    stats_.storedEntries = 0;
    return !ec;
  }

} // namespace ailivingmaze::shader
