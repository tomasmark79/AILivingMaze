#pragma once

#include "World/World.hpp"

#include <glm/glm.hpp>

namespace ailivingmaze::game {

  class CollisionSystem {
  public:
    explicit CollisionSystem(const world::World &world) noexcept : world_(world) {}

    [[nodiscard]] glm::vec3 resolveMovement(const glm::vec3 &fromPos, const glm::vec3 &toPos,
                                            float radius) const noexcept;

  private:
    static void pushOutFromAABB2D(glm::vec2 &pos, float radius, const glm::vec2 &boxMin,
                                  const glm::vec2 &boxMax) noexcept;

    const world::World &world_;
  };

} // namespace ailivingmaze::game
