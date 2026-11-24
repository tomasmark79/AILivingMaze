#pragma once

#include <Utils/UtilsFactory.hpp>

#include <memory>

namespace ailivingmaze::v1 {
  using namespace ailivingmaze::utils;
  class AILivingMazeLib {

  public:
    AILivingMazeLib(const UtilsFactory::ApplicationContext &context);
    ~AILivingMazeLib();
    AILivingMazeLib(const AILivingMazeLib &other) = delete;
    AILivingMazeLib &operator=(const AILivingMazeLib &other) = delete;
    AILivingMazeLib(AILivingMazeLib &&other) = delete;
    AILivingMazeLib &operator=(AILivingMazeLib &&other) = delete;

    [[nodiscard]] bool isInitialized() const noexcept;
    [[nodiscard]]
    const std::shared_ptr<ailivingmaze::assets::IAssetManager> &getAssetManager() const noexcept;
    void performLibraryTask();

  private:
    bool isInitialized_{false};
    static constexpr const char *libName_ = "AILivingMazeLib v0.0.1";

    std::shared_ptr<ailivingmaze::logging::ILogger> logger_;
    std::shared_ptr<ailivingmaze::assets::IAssetManager> assetManager_;
  };

} // namespace ailivingmaze::v1
