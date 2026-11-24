#include "Game/Player.hpp"

#include "Game/CollisionSystem.hpp"

#include <glm/glm.hpp>

namespace ailivingmaze::game {

  Player::Player(const glm::vec3 &spawnPos) : position_(spawnPos) {
    position_.y = eyeHeight_;
    camera_.setPosition(position_);
  }

  void Player::update(float dt, const engine::InputState &input, const CollisionSystem &collision) {
    if (input.mouseDeltaX != 0.0F || input.mouseDeltaY != 0.0F) {
      camera_.addLookDelta(input.mouseDeltaX, input.mouseDeltaY);
    }

    glm::vec3 moveDir{0.0F};
    if (input.forward) {
      moveDir += camera_.forwardXZ();
    }
    if (input.backward) {
      moveDir -= camera_.forwardXZ();
    }
    if (input.right) {
      moveDir += camera_.rightXZ();
    }
    if (input.left) {
      moveDir -= camera_.rightXZ();
    }

    if (glm::dot(moveDir, moveDir) > 1e-6F) {
      moveDir = glm::normalize(moveDir);
      const float speed = input.sprint ? sprintSpeed_ : walkSpeed_;
      const glm::vec3 desired = position_ + moveDir * (speed * dt);
      position_ = collision.resolveMovement(position_, desired, radius_);
      position_.y = eyeHeight_;
    }

    camera_.setPosition(position_);
  }

} // namespace ailivingmaze::game
