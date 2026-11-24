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
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>

namespace ailivingmaze::engine {

  namespace {
    struct RuntimeSession {
      std::shared_ptr<logging::ILogger> logger;
      EngineConfig config;
      Window window;
      shader::ShaderManager shaderManager{24};
      render::DebugOverlay debugOverlay;
      shader::MaterialManager materialManager;
      shader::ShaderCache shaderCache;
      std::shared_ptr<ai::IAIProvider> provider;
      std::unique_ptr<ai::AIShaderService> aiService;
      world::World world;
      game::Player player;
      game::CollisionSystem collision;
      render::Renderer renderer;
      Input input;
      world::SectorCoord lastSector{};
      std::chrono::steady_clock::time_point startTime{};
      std::chrono::steady_clock::time_point prevTime{};
      float smoothFps = 60.0F;
      float smoothFrameMs = 16.6F;
      bool running = true;

      RuntimeSession(std::shared_ptr<logging::ILogger> log, EngineConfig cfg)
          : logger(std::move(log)), config(std::move(cfg)), materialManager(shaderManager),
            shaderCache(config.shaderCacheDir), world(config.worldSeed, config.chunkLoadRadius),
            player(world::World::defaultSpawnPosition()), collision(world) {}

      bool init() {
        auto &log = *logger;
        std::string err;
        if (!window.init(WindowConfig{.title = "AI Living Maze",
                                      .width = config.windowWidth,
                                      .height = config.windowHeight,
                                      .vsync = config.vsync,
                                      .resizable = true},
                         err)) {
          log.errorStream() << "Window/OpenGL init failed: " << err;
          return false;
        }

        log.infoStream() << "OpenGL Renderer: " << window.glRendererString()
                         << " | Version: " << window.glVersionString();

        if (!shaderManager.init(err)) {
          log.errorStream() << "ShaderManager init failed: " << err;
          return false;
        }

        if (!debugOverlay.init(err)) {
          log.warningStream() << "DebugOverlay init failed (continuing without HUD): " << err;
        }

        if (config.aiConfig.enabled) {
          if (config.aiConfig.providerType == "openai") {
            provider = std::make_shared<ai::OpenAICompatibleProvider>(config.aiConfig);
            if (!provider->isAvailable()) {
              log.warningStream() << "OpenAI-compatible provider unavailable (check "
                                  << config.aiConfig.apiKeyEnvVar
                                  << "); running with cached/default shaders.";
            }
          } else if (config.aiConfig.providerType == "mock") {
            provider = std::make_shared<ai::MockAIProvider>(config.aiConfig.mockLatencyMs);
          }
        }

        aiService = std::make_unique<ai::AIShaderService>(config.aiConfig, provider, shaderCache);
        aiService->start();

        player.camera().setInvertY(config.invertMouseY);
#ifndef __EMSCRIPTEN__
        window.setRelativeMouseMode(true);
#endif

        world.update(player.position());
        renderer.syncChunks(world.chunkManager());

        lastSector = world::SectorManager::sectorForWorldPos(player.position());
        aiService->pregenerateAround(world.sectorManager(), lastSector);

        startTime = std::chrono::steady_clock::now();
        prevTime = startTime;
        return true;
      }

      void step() {
        auto &log = *logger;
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
#ifdef __EMSCRIPTEN__
          emscripten_cancel_main_loop();
#endif
          return;
        }
        if (st.windowResized) {
          window.updateDimensions();
          glViewport(0, 0, window.width(), window.height());
        }
#ifdef __EMSCRIPTEN__
        {
          const int prevW = window.width();
          const int prevH = window.height();
          window.updateDimensions();
          if (window.width() != prevW || window.height() != prevH) {
            glViewport(0, 0, window.width(), window.height());
          }
        }
#endif
        if (st.toggleCursorCapture) {
          window.setRelativeMouseMode(!window.isRelativeMouseMode());
        }
        if (st.toggleDebugOverlay) {
          debugOverlay.toggleVisible();
        }
        if (st.toggleShaderMode) {
          materialManager.toggleShaderMode();
          log.infoStream() << "Shader mode switched to: "
                           << (materialManager.usingAIShaders() ? "AI-Generated"
                                                                : "Default Fallback");
        }
        if (st.toggleInvertMouseY) {
          player.camera().toggleInvertY();
          log.infoStream() << "Vertical mouse inversion: "
                           << (player.camera().isInvertY() ? "ON" : "OFF");
        }

        player.update(dt, st, collision);
        world.update(player.position());
        renderer.syncChunks(world.chunkManager());

        const auto curSector = world::SectorManager::sectorForWorldPos(player.position());
        const auto curTheme = world.sectorManager().describeSector(curSector);

        if (!(curSector == lastSector)) {
          lastSector = curSector;
          aiService->pregenerateAround(world.sectorManager(), curSector);
          log.infoStream() << "Entered " << curTheme.sectorId << " (" << curTheme.styleName << ")";
        }

        if (st.requestNewShader) {
          log.infoStream() << "F2 pressed: requesting new AI shader for " << curTheme.sectorId;
          aiService->requestSector(curTheme, true);
        }

        // Poll completed async AI shader responses and compile/hot-reload on OpenGL thread
        for (const auto &resp : aiService->pollCompleted()) {
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
        hud.invertMouseY = player.camera().isInvertY();
        hud.aiProviderStatus = aiService->providerStatusString();
        hud.pendingAIRequests = aiService->pendingRequestCount();
        hud.lastGenerationMs = aiService->lastGenerationMs();
        hud.lastCompileMs = shaderManager.lastCompileMs();
        hud.cacheHits = cacheStats.hits;
        hud.cacheMisses = cacheStats.misses;
        hud.cacheStored = cacheStats.storedEntries;
        hud.lastShaderError = shaderManager.lastError();

        debugOverlay.render(window.width(), window.height(), hud);
        window.swapBuffers();
      }

      void shutdown() {
        if (aiService) {
          aiService->stop();
        }
        renderer.shutdown();
        debugOverlay.shutdown();
        shaderManager.shutdown();
        window.shutdown();
      }
    };
  } // namespace

  MazeApplication::MazeApplication(const utils::UtilsFactory::ApplicationContext &context,
                                   EngineConfig config)
      : logger_(context.logger ? context.logger : std::make_shared<logging::NullLogger>()),
        config_(std::move(config)) {}

  int MazeApplication::run() {
    auto session = std::make_unique<RuntimeSession>(logger_, config_);
    if (!session->init()) {
      session->shutdown();
      return EXIT_FAILURE;
    }

#ifdef __EMSCRIPTEN__
    // Hand ownership to Emscripten's browser animation loop (simulate_infinite_loop = false
    // avoids unwinding C++ stack frames when exception catching is enabled).
    RuntimeSession *rawSession = session.release();
    emscripten_set_main_loop_arg(
        [](void *arg) {
          auto *s = static_cast<RuntimeSession *>(arg);
          s->step();
          if (!s->running) {
            s->shutdown();
            delete s;
          }
        },
        rawSession, 0, false);
    return EXIT_SUCCESS;
#else
    while (session->running) {
      session->step();
    }
    session->shutdown();
    return EXIT_SUCCESS;
#endif
  }

} // namespace ailivingmaze::engine
