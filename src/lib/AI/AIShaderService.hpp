#pragma once

#include "AI/AIProvider.hpp"
#include "Shader/ShaderCache.hpp"
#include "World/SectorManager.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ailivingmaze::ai {

  class AIShaderService {
  public:
    AIShaderService(AIConfig config, std::shared_ptr<IAIProvider> provider,
                    shader::ShaderCache &cache);
    ~AIShaderService();

    AIShaderService(const AIShaderService &) = delete;
    AIShaderService &operator=(const AIShaderService &) = delete;

    void start();
    void stop();

    // Enqueues a shader request for a sector (uses disk cache unless forceRegenerate is true).
    void requestSector(const world::SectorTheme &theme, bool forceRegenerate = false);

    // Pre-generates shaders for surrounding sectors within configured pregenerateRadius.
    void pregenerateAround(const world::SectorManager &sectorMgr, world::SectorCoord center);

    // Non-blocking poll called from the main/OpenGL thread.
    [[nodiscard]] std::vector<AIShaderResponse> pollCompleted();

    [[nodiscard]] std::string providerStatusString() const;
    [[nodiscard]] std::size_t pendingRequestCount() const;
    [[nodiscard]] double lastGenerationMs() const noexcept { return lastGenerationMs_.load(); }
    [[nodiscard]] bool isEnabled() const noexcept {
      return config_.enabled && provider_ != nullptr && provider_->isAvailable();
    }

  private:
    struct Job {
      ShaderPromptContext ctx;
      bool forceRegenerate = false;
    };

    void workerLoop();

    AIConfig config_;
    std::shared_ptr<IAIProvider> provider_;
    shader::ShaderCache &cache_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Job> queue_;
    std::unordered_set<std::string> inFlightSectors_;
    std::unordered_set<std::string> requestedSectors_;
    std::unordered_map<std::string, uint32_t> sectorVariations_;
    std::vector<AIShaderResponse> completed_;
    std::vector<std::thread> workers_;
    bool running_ = false;
    std::atomic<double> lastGenerationMs_{0.0};
  };

} // namespace ailivingmaze::ai
