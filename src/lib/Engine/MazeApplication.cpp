#include "Engine/MazeApplication.hpp"

#include "AI/AIShaderService.hpp"
#include "AI/MockAIProvider.hpp"
#include "AI/OpenAICompatibleProvider.hpp"
#include "Engine/Input.hpp"
#include "Engine/Window.hpp"
#include "Game/CollisionSystem.hpp"
#include "Game/Player.hpp"
#include "Render/DebugOverlay.hpp"
#include "Render/Renderer.hpp"
#include "Shader/MaterialManager.hpp"
#include "Shader/ShaderCache.hpp"
#include "Shader/ShaderManager.hpp"
#include "World/World.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>

namespace ailivingmaze::engine {

  MazeApplication::MazeApplication(const utils::UtilsFactory::ApplicationContext &context,
                                   EngineConfig config)
      : logger_(context.logger ? context.logger : std::make_shared<logging::NullLogger>()),
        config_(std::move(config)) {}

  int MazeApplication::run() {
    auto &log = *logger_;

    Window window;
    std::string err;
    if (!window.init(WindowConfig{.title = "AI Living Maze",
                                  .width = config_.windowWidth,
                                  .height = config_.windowHeight,
                                  .vsync = config_.vsync,
                                  .resizable = true},
                     err)) {
      log.errorStream() << "Window/OpenGL init failed: " << err;
      return EXIT_FAILURE;
    }

    log.infoStream() << "OpenGL Renderer: " << window.glRendererString()
                     << " | Version: " << window.glVersionString();

    shader::ShaderManager shaderManager(24);
    if (!shaderManager.init(err)) {
      log.errorStream() << "ShaderManager init failed: " << err;
      return EXIT_FAILURE;
    }

    render::DebugOverlay debugOverlay;
    if (!debugOverlay.init(err)) {
      log.warningStream() << "DebugOverlay init failed (continuing without HUD): " << err;
    }

    shader::MaterialManager materialManager(shaderManager);
    shader::ShaderCache shaderCache(config_.shaderCacheDir);

    std::shared_ptr<ai::IAIProvider> provider;
    if (config_.aiConfig.enabled) {
      if (config_.aiConfig.providerType == "openai") {
        provider = std::make_shared<ai::OpenAICompatibleProvider>(config_.aiConfig);
        if (!provider->isAvailable()) {
          log.warningStream() << "OpenAI-compatible provider unavailable (check "
                              << config_.aiConfig.apiKeyEnvVar
                              << "); running with cached/default shaders.";
        }
      } else if (config_.aiConfig.providerType == "mock") {
        provider = std::make_shared<ai::MockAIProvider>(config_.aiConfig.mockLatencyMs);
      }
    }

    ai::AIShaderService aiService(config_.aiConfig, provider, shaderCache);
    aiService.start();

    world::World world(config_.worldSeed, config_.chunkLoadRadius);
    game::Player player(world::World::defaultSpawnPosition());
    game::CollisionSystem collision(world);
    render::Renderer renderer;
    Input input;

    window.setRelativeMouseMode(true);

    world.update(player.position());
    renderer.syncChunks(world.chunkManager());

    auto lastSector = world::SectorManager::sectorForWorldPos(player.position());
    aiService.pregenerateAround(world.sectorManager(), lastSector);

    const auto startTime = std::chrono::steady_clock::now();
    auto prevTime = startTime;
    float smoothFps = 60.0F;
    float smoothFrameMs = 16.6F;

    bool running = true;
    while (running) {
      const auto now = std::chrono::steady_clock::now();
      const float rawDt = std::chrono::duration<float>(now - prevTime).count();
      prevTime = now;
      const float dt = std::clamp(rawDt, 0.0001F, 0.1F);
      const float elapsed = std::chrono::duration<float>(now - startTime).count();

      const float instFrameMs = rawDt * 1000.0F;
      const float instFps = rawDt > 1e-5F ? (1.0F / rawDt) : 0.0F;
      smoothFrameMs = smoothFrameMs * 0.92F + instFrameMs * 0.08F;
      smoothFps = smoothFps * 0.92F + instFps * 0.08F;

      input.beginFrame();
      SDL_Event ev;
      while (SDL_PollEvent(&ev)) {
        input.processEvent(ev, window.isRelativeMouseMode());
      }

      const auto &st = input.state();
      if (st.quitRequested) {
        running = false;
        break;
      }
      if (st.windowResized) {
        window.updateDimensions();
        glViewport(0, 0, window.width(), window.height());
      }
      if (st.toggleCursorCapture) {
        window.setRelativeMouseMode(!window.isRelativeMouseMode());
      }
      if (st.toggleDebugOverlay) {
        debugOverlay.toggleVisible();
      }
      if (st.toggleShaderMode) {
        materialManager.toggleShaderMode();
        log.infoStream() << "Shader mode switched to: "
                         << (materialManager.usingAIShaders() ? "AI-Generated" : "Default Fallback");
      }

      player.update(dt, st, collision);
      world.update(player.position());
      renderer.syncChunks(world.chunkManager());

      const auto curSector = world::SectorManager::sectorForWorldPos(player.position());
      const auto curTheme = world.sectorManager().describeSector(curSector);

      if (!(curSector == lastSector)) {
        lastSector = curSector;
        aiService.pregenerateAround(world.sectorManager(), curSector);
        log.infoStream() << "Entered " << curTheme.sectorId << " (" << curTheme.styleName << ")";
      }

      if (st.requestNewShader) {
        log.infoStream() << "F2 pressed: requesting new AI shader for " << curTheme.sectorId;
        aiService.requestSector(curTheme, true);
      }

      // Poll completed async AI shader responses and compile/hot-reload on OpenGL thread
      for (const auto &resp : aiService.pollCompleted()) {
        if (!resp.success) {
          log.warningStream() << "AI shader generation failed for " << resp.sectorId << ": "
                              << resp.errorMessage;
          continue;
        }
        auto compileRes = shaderManager.compileAndActivate(
            resp.sectorId, resp.glslCode, resp.modelUsed, resp.fromCache ? nullptr : &shaderCache,
            resp.promptUsed, resp.generationMs);
        if (compileRes.success) {
          log.infoStream() << "Hot-reloaded shader for " << resp.sectorId << " ("
                           << (resp.fromCache ? "cache" : resp.modelUsed) << ", compile="
                           << compileRes.compileMs << "ms, probe=" << compileRes.probeMs << "ms)";
        } else {
          log.errorStream() << "Shader validation/compile failed for " << resp.sectorId << ": "
                            << compileRes.errorMessage;
        }
      }

      const auto renderStats =
          renderer.render(world, player.camera(), materialManager, window.aspectRatio(), elapsed);

      const auto curChunk = world::ChunkManager::chunkForWorldPos(player.position());
      const auto cacheStats = shaderCache.stats();

      render::DebugOverlayData hud{};
      hud.fps = smoothFps;
      hud.frameTimeMs = smoothFrameMs;
      hud.playerPos = player.position();
      hud.chunkX = curChunk.x;
      hud.chunkZ = curChunk.z;
      hud.sectorId = curTheme.sectorId;
      hud.sectorStyle = curTheme.styleName;
      hud.loadedChunks = renderStats.loadedChunks;
      hud.renderedChunks = renderStats.renderedChunks;
      hud.drawCalls = renderStats.drawCalls;
      hud.activeShaders = shaderManager.activeProgramCount();
      hud.usingAIShaders = materialManager.usingAIShaders();
      hud.cursorCaptured = window.isRelativeMouseMode();
      hud.aiProviderStatus = aiService.providerStatusString();
      hud.pendingAIRequests = aiService.pendingRequestCount();
      hud.lastGenerationMs = aiService.lastGenerationMs();
      hud.lastCompileMs = shaderManager.lastCompileMs();
      hud.cacheHits = cacheStats.hits;
      hud.cacheMisses = cacheStats.misses;
      hud.cacheStored = cacheStats.storedEntries;
      hud.lastShaderError = shaderManager.lastError();

      debugOverlay.render(window.width(), window.height(), hud);
      window.swapBuffers();
    }

    aiService.stop();
    renderer.shutdown();
    debugOverlay.shutdown();
    shaderManager.shutdown();
    window.shutdown();
    return EXIT_SUCCESS;
  }

} // namespace ailivingmaze::engine
