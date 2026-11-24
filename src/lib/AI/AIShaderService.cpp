#include "AI/AIShaderService.hpp"

#include <algorithm>

namespace ailivingmaze::ai {

  AIShaderService::AIShaderService(AIConfig config, std::shared_ptr<IAIProvider> provider,
                                   shader::ShaderCache &cache)
      : config_(std::move(config)), provider_(std::move(provider)), cache_(cache) {}

  AIShaderService::~AIShaderService() { stop(); }

  void AIShaderService::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
      return;
    }
    running_ = true;
    const int workerCount = std::clamp(config_.maxParallelRequests, 1, 4);
    workers_.reserve(static_cast<std::size_t>(workerCount));
    for (int i = 0; i < workerCount; ++i) {
      workers_.emplace_back(&AIShaderService::workerLoop, this);
    }
  }

  void AIShaderService::stop() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!running_) {
        return;
      }
      running_ = false;
      queue_.clear();
    }
    cv_.notify_all();
    for (auto &t : workers_) {
      if (t.joinable()) {
        t.join();
      }
    }
    workers_.clear();
  }

  void AIShaderService::requestSector(const world::SectorTheme &theme, bool forceRegenerate) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
      return;
    }
    if (inFlightSectors_.contains(theme.sectorId)) {
      return;
    }
    if (!forceRegenerate && requestedSectors_.contains(theme.sectorId)) {
      return;
    }

    uint32_t varSeed = sectorVariations_[theme.sectorId];
    if (forceRegenerate) {
      ++varSeed;
      sectorVariations_[theme.sectorId] = varSeed;
    }

    inFlightSectors_.insert(theme.sectorId);
    requestedSectors_.insert(theme.sectorId);
    queue_.push_back(Job{ShaderPromptContext{theme, varSeed}, forceRegenerate});
    cv_.notify_one();
  }

  void AIShaderService::pregenerateAround(const world::SectorManager &sectorMgr,
                                          world::SectorCoord center) {
    const auto sectors = sectorMgr.nearbySectors(center, std::max(0, config_.pregenerateRadius));
    for (const auto &sc : sectors) {
      const auto theme = sectorMgr.describeSector(sc);
      requestSector(theme, false);
    }
  }

  std::vector<AIShaderResponse> AIShaderService::pollCompleted() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AIShaderResponse> out;
    out.swap(completed_);
    return out;
  }

  std::string AIShaderService::providerStatusString() const {
    if (!config_.enabled || !provider_) {
      return "Offline (Disabled)";
    }
    if (!provider_->isAvailable()) {
      return provider_->name() + " (Unavailable / Missing Key)";
    }
    return provider_->name() + " [" + provider_->modelName() + "]";
  }

  std::size_t AIShaderService::pendingRequestCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return inFlightSectors_.size();
  }

  void AIShaderService::workerLoop() {
    while (true) {
      Job job{};
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return !running_ || !queue_.empty(); });
        if (!running_ && queue_.empty()) {
          return;
        }
        job = std::move(queue_.front());
        queue_.pop_front();
      }

      AIShaderResponse response{};
      response.sectorId = job.ctx.theme.sectorId;

      // 1. Prefer valid cached shader on startup / normal traversal
      if (!job.forceRegenerate) {
        if (auto cached = cache_.loadLatestForSector(job.ctx.theme.sectorId); cached.has_value()) {
          response.success = true;
          response.glslCode = std::move(cached->glslSource);
          response.promptUsed = std::move(cached->prompt);
          response.modelUsed = std::move(cached->aiModel);
          response.generationMs = cached->generationMs;
          response.fromCache = true;
        }
      }

      // 2. Otherwise query AI provider if enabled & available
      if (!response.success) {
        if (config_.enabled && provider_ && provider_->isAvailable()) {
          response = provider_->generateShader(job.ctx);
          if (response.success) {
            lastGenerationMs_.store(response.generationMs);
          }
        } else {
          response.success = false;
          response.errorMessage = "AI provider offline; keeping default shader";
        }
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        inFlightSectors_.erase(job.ctx.theme.sectorId);
        completed_.push_back(std::move(response));
      }
    }
  }

} // namespace ailivingmaze::ai
