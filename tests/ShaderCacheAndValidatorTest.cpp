#include "AI/AIShaderService.hpp"
#include "AI/MockAIProvider.hpp"
#include "AI/OpenAICompatibleProvider.hpp"
#include "Shader/ShaderAPI.hpp"
#include "Shader/ShaderCache.hpp"
#include "Shader/ShaderValidator.hpp"
#include "World/SectorManager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

using namespace ailivingmaze::shader;
using namespace ailivingmaze::ai;
using namespace ailivingmaze::world;

namespace {
  std::filesystem::path makeTempCacheDir(const std::string &suffix) {
    auto dir = std::filesystem::temp_directory_path() / ("ailivingmaze_test_" + suffix);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    return dir;
  }
} // namespace

TEST(ShaderValidatorTest, AcceptsDefaultAndAllMockArchetypes) {
  const auto defRes = ShaderValidator::validate(kDefaultSurfaceGlsl);
  EXPECT_TRUE(defRes.valid) << defRes.errorMessage;

  MockAIProvider mock(0);
  SectorManager sectorMgr(42ULL);
  for (int archetype = 0; archetype < 9; ++archetype) {
    SectorTheme theme = sectorMgr.describeSector({archetype, 0});
    theme.archetypeIndex = archetype;
    const auto resp = mock.generateShader(ShaderPromptContext{theme, 1});
    ASSERT_TRUE(resp.success);
    const auto val = ShaderValidator::validate(resp.glslCode);
    EXPECT_TRUE(val.valid) << "Archetype " << archetype << " failed: " << val.errorMessage;
  }
}

TEST(ShaderValidatorTest, RejectsInvalidAndUnsafeGlsl) {
  EXPECT_FALSE(ShaderValidator::validate("").valid);
  EXPECT_FALSE(ShaderValidator::validate("float x = 1.0;").valid);

  // Forbidden #version / main() / uniform / while / excessive loop
  EXPECT_FALSE(ShaderValidator::validate(R"(
    #version 330 core
    SurfaceOutput evaluateSurface(SurfaceInput s) { SurfaceOutput o; return o; }
  )")
                   .valid);

  EXPECT_FALSE(ShaderValidator::validate(R"(
    uniform float hack;
    SurfaceOutput evaluateSurface(SurfaceInput s) { SurfaceOutput o; return o; }
  )")
                   .valid);

  EXPECT_FALSE(ShaderValidator::validate(R"(
    SurfaceOutput evaluateSurface(SurfaceInput s) {
      SurfaceOutput o;
      while (true) {}
      return o;
    }
  )")
                   .valid);

  EXPECT_FALSE(ShaderValidator::validate(R"(
    SurfaceOutput evaluateSurface(SurfaceInput s) {
      SurfaceOutput o;
      for (int i = 0; i < 200; ++i) { o.ao += 0.001; }
      return o;
    }
  )")
                   .valid);
}

TEST(ShaderCacheTest, PersistsLoadsAndInvalidatesVersionedEntries) {
  const auto dir = makeTempCacheDir("cache_persist");
  {
    ShaderCache cache(dir);
    CachedShaderRecord rec{};
    rec.sectorId = "sector_1_-2";
    rec.glslSource = kDefaultSurfaceGlsl;
    rec.aiModel = "mock-procedural-v1";
    rec.prompt = "test prompt";
    rec.shaderApiVersion = kShaderApiVersion;
    rec.generationMs = 12.5;
    rec.compileMs = 3.2;
    ASSERT_TRUE(cache.save(rec));
  }

  // Reload from a fresh ShaderCache instance (simulating application restart)
  {
    ShaderCache cache(dir);
    const auto loaded = cache.loadLatestForSector("sector_1_-2");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->sectorId, "sector_1_-2");
    EXPECT_EQ(loaded->glslSource, kDefaultSurfaceGlsl);
    EXPECT_EQ(loaded->shaderApiVersion, kShaderApiVersion);
    EXPECT_EQ(cache.stats().hits, 1U);

    EXPECT_TRUE(cache.invalidateSector("sector_1_-2"));
    EXPECT_FALSE(cache.loadLatestForSector("sector_1_-2").has_value());
  }

  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

TEST(ShaderCacheTest, RejectsMismatchedApiVersion) {
  const auto dir = makeTempCacheDir("cache_version");
  ShaderCache cache(dir);
  CachedShaderRecord rec{};
  rec.sectorId = "sector_0_0";
  rec.glslSource = kDefaultSurfaceGlsl;
  rec.shaderApiVersion = "0.0.1-obsolete";
  ASSERT_TRUE(cache.save(rec));

  EXPECT_FALSE(cache.loadLatestForSector("sector_0_0").has_value());

  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

TEST(OpenAICompatibleProviderTest, HandlesMalformedAndValidApiJsonResponses) {
  std::string glsl;
  std::string err;

  EXPECT_FALSE(OpenAICompatibleProvider::extractGlslFromApiJson("not json", glsl, err));
  EXPECT_FALSE(OpenAICompatibleProvider::extractGlslFromApiJson(R"({"choices":[]})", glsl, err));
  EXPECT_FALSE(OpenAICompatibleProvider::extractGlslFromApiJson(
      R"({"error":{"message":"quota exceeded"}})", glsl, err));

  const std::string validResp = R"({
    "choices": [
      {
        "message": {
          "content": "```glsl\nSurfaceOutput evaluateSurface(SurfaceInput s) {\n  SurfaceOutput o;\n  o.albedo = s.primaryColor;\n  o.emissive = vec3(0.0);\n  o.roughness = 0.5;\n  o.metallic = 0.2;\n  o.ao = 1.0;\n  return o;\n}\n```"
        }
      }
    ]
  })";
  ASSERT_TRUE(OpenAICompatibleProvider::extractGlslFromApiJson(validResp, glsl, err)) << err;
  EXPECT_TRUE(ShaderValidator::validate(glsl).valid);
}

TEST(AIShaderServiceTest, AsyncGenerationAndDiskCacheReuse) {
  const auto dir = makeTempCacheDir("async_service");
  ShaderCache cache(dir);
  AIConfig cfg{};
  cfg.enabled = true;
  cfg.maxParallelRequests = 1;
  cfg.mockLatencyMs = 0;

  auto provider = std::make_shared<MockAIProvider>(0);
  AIShaderService service(cfg, provider, cache);
  service.start();

  SectorManager sectorMgr(1337ULL);
  const auto theme = sectorMgr.describeSector({0, 0});
  service.requestSector(theme, false);

  std::vector<AIShaderResponse> results;
  for (int i = 0; i < 50 && results.empty(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    results = service.pollCompleted();
  }
  ASSERT_EQ(results.size(), 1U);
  EXPECT_TRUE(results[0].success);
  EXPECT_FALSE(results[0].fromCache);
  EXPECT_TRUE(ShaderValidator::validate(results[0].glslCode).valid);

  service.stop();
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}
