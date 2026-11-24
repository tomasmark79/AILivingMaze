#include <AILivingMazeLib/AILivingMazeLib.hpp>
#include <Engine/MazeApplication.hpp>
#include <Utils/UtilsFactory.hpp>
#include <cxxopts.hpp>
#include <iostream>

const std::string appName = "AILivingMaze";

int main(int argc, char **argv) {
  using namespace ailivingmaze;
  using namespace ailivingmaze::logging;
  using namespace ailivingmaze::utils;

  try {
    cxxopts::Options options(
        appName, "AI Living Maze - Experimental 3D engine with runtime AI-generated GLSL shaders");
    options.add_options()
        ("h,help", "Print usage")
        ("w,write2file", "Write log output to file", cxxopts::value<bool>()->default_value("false"))
        ("seed", "Deterministic world seed", cxxopts::value<uint64_t>()->default_value("1337"))
        ("chunk-radius", "Chunk load radius around player", cxxopts::value<int>()->default_value("2"))
        ("width", "Window width", cxxopts::value<int>()->default_value("1280"))
        ("height", "Window height", cxxopts::value<int>()->default_value("720"))
        ("no-vsync", "Disable VSync", cxxopts::value<bool>()->default_value("false"))
        ("invert-mouse-y", "Invert vertical mouse look",
         cxxopts::value<bool>()->default_value("false"))
        ("ai-provider", "AI provider: mock | openai | none",
         cxxopts::value<std::string>()->default_value("mock"))
        ("ai-endpoint", "OpenAI-compatible API endpoint URL",
         cxxopts::value<std::string>()->default_value("https://api.openai.com/v1/chat/completions"))
        ("ai-model", "AI model name", cxxopts::value<std::string>()->default_value("gpt-4o-mini"))
        ("ai-key-env", "Environment variable holding the API key",
         cxxopts::value<std::string>()->default_value("OPENAI_API_KEY"))
        ("ai-timeout", "HTTP request timeout in seconds", cxxopts::value<int>()->default_value("25"))
        ("ai-parallel", "Max parallel AI requests", cxxopts::value<int>()->default_value("2"))
        ("ai-pregen-radius", "Sector pregeneration radius", cxxopts::value<int>()->default_value("1"))
        ("cache-dir", "Persistent shader cache directory",
         cxxopts::value<std::string>()->default_value(".cache/ailivingmaze/shaders"));

    auto result = options.parse(argc, argv);

    if (result.contains("help")) {
      std::cout << options.help() << '\n';
      return EXIT_SUCCESS;
    }

    auto ctx = UtilsFactory::createFullContext(
        appName, LoggerConfig{.level = Level::LOG_INFO,
                              .enableFileLogging = result["write2file"].as<bool>(),
                              .logFilePath = "ailivingmaze.log",
                              .colorOutput = true,
                              .appPrefix = appName});

    ctx.logger->infoStream() << appName << " starting on " << ctx.platformInfo->getPlatformName();

    auto library = std::make_unique<ailivingmaze::v1::AILivingMazeLib>(ctx);
    if (!library->isInitialized()) {
      ctx.logger->errorStream() << "Library initialization failed";
      return EXIT_FAILURE;
    }

    engine::EngineConfig cfg{};
    cfg.worldSeed = result["seed"].as<uint64_t>();
    cfg.chunkLoadRadius = result["chunk-radius"].as<int>();
    cfg.windowWidth = result["width"].as<int>();
    cfg.windowHeight = result["height"].as<int>();
    cfg.vsync = !result["no-vsync"].as<bool>();
    cfg.invertMouseY = result["invert-mouse-y"].as<bool>();
    cfg.shaderCacheDir = result["cache-dir"].as<std::string>();

    cfg.aiConfig.providerType = result["ai-provider"].as<std::string>();
    cfg.aiConfig.enabled = (cfg.aiConfig.providerType != "none");
    cfg.aiConfig.endpoint = result["ai-endpoint"].as<std::string>();
    cfg.aiConfig.model = result["ai-model"].as<std::string>();
    cfg.aiConfig.apiKeyEnvVar = result["ai-key-env"].as<std::string>();
    cfg.aiConfig.timeoutSeconds = result["ai-timeout"].as<int>();
    cfg.aiConfig.maxParallelRequests = result["ai-parallel"].as<int>();
    cfg.aiConfig.pregenerateRadius = result["ai-pregen-radius"].as<int>();

    engine::MazeApplication app(ctx, std::move(cfg));
    const int exitCode = app.run();

    ctx.logger->infoStream() << appName << " ... shutting down";
    return exitCode;

  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
