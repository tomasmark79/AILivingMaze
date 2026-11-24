#pragma once

#include "Engine/Input.hpp"
#include "Render/Camera.hpp"

#include <glm/glm.hpp>

namespace ailivingmaze::game {

  class CollisionSystem;

  class Player {
  public:
    explicit Player(const glm::vec3 &spawnPos = {2.0F, 1.7F, 2.0F});

    void update(float dt, const engine::InputState &input, const CollisionSystem &collision);

    [[nodiscard]] const glm::vec3 &position() const noexcept { return position_; }
    [[nodiscard]] float radius() const noexcept { return radius_; }
    [[nodiscard]] float eyeHeight() const noexcept { return eyeHeight_; }
    [[nodiscard]] render::Camera &camera() noexcept { return camera_; }
    [[nodiscard]] const render::Camera &camera() const noexcept { return camera_; }

  private:
    glm::vec3 position_{2.0F, 1.7F, 2.0F};
    float eyeHeight_ = 1.7F;
    float radius_ = 0.55F;
    float walkSpeed_ = 6.5F;
    float sprintSpeed_ = 13.0F;
    render::Camera camera_{};
  };

} // namespace ailivingmaze::game
