#include "Game/CollisionSystem.hpp"

#include <algorithm>
#include <cmath>

namespace ailivingmaze::game {

  void CollisionSystem::pushOutFromAABB2D(glm::vec2 &pos, float radius, const glm::vec2 &boxMin,
                                          const glm::vec2 &boxMax) noexcept {
    const glm::vec2 closest{std::clamp(pos.x, boxMin.x, boxMax.x),
                            std::clamp(pos.y, boxMin.y, boxMax.y)};
    const glm::vec2 diff = pos - closest;
    const float distSq = glm::dot(diff, diff);

    if (distSq >= radius * radius) {
      return;
    }

    if (distSq > 1e-8F) {
      const float dist = std::sqrt(distSq);
      pos += (diff / dist) * (radius - dist);
    } else {
      // Center is inside the box: push out along shallowest axis
      const float dxLeft = pos.x - boxMin.x;
      const float dxRight = boxMax.x - pos.x;
      const float dzTop = pos.y - boxMin.y;
      const float dzBottom = boxMax.y - pos.y;
      const float minPen = std::min({dxLeft, dxRight, dzTop, dzBottom});
      if (minPen == dxLeft) {
        pos.x = boxMin.x - radius;
      } else if (minPen == dxRight) {
        pos.x = boxMax.x + radius;
      } else if (minPen == dzTop) {
        pos.y = boxMin.y - radius;
      } else {
        pos.y = boxMax.y + radius;
      }
    }
  }

  glm::vec3 CollisionSystem::resolveMovement(const glm::vec3 &fromPos, const glm::vec3 &toPos,
                                             float radius) const noexcept {
    // Substep movement to prevent tunneling at high speed or low FPS
    const glm::vec2 start{fromPos.x, fromPos.z};
    const glm::vec2 end{toPos.x, toPos.z};
    const float dist = glm::length(end - start);
    const int steps = std::max(1, static_cast<int>(std::ceil(dist / (radius * 0.5F))));

    glm::vec2 current = start;
    const glm::vec2 stepDelta = (end - start) / static_cast<float>(steps);
    const float halfT = world::kWallThickness * 0.5F;

    for (int s = 0; s < steps; ++s) {
      current += stepDelta;

      const int32_t cellX = static_cast<int32_t>(std::floor(current.x / world::kCellSize));
      const int32_t cellZ = static_cast<int32_t>(std::floor(current.y / world::kCellSize));

      // Check walls of the 3x3 neighborhood around current cell
      for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
          const int32_t gx = cellX + dx;
          const int32_t gz = cellZ + dz;
          const uint8_t mask = world_.cellWallsAtGlobal(gx, gz);
          const float x0 = static_cast<float>(gx) * world::kCellSize;
          const float z0 = static_cast<float>(gz) * world::kCellSize;
          const float x1 = x0 + world::kCellSize;
          const float z1 = z0 + world::kCellSize;

          if ((mask & world::WALL_NORTH) != 0) {
            pushOutFromAABB2D(current, radius, {x0 - halfT, z0 - halfT}, {x1 + halfT, z0 + halfT});
          }
          if ((mask & world::WALL_SOUTH) != 0) {
            pushOutFromAABB2D(current, radius, {x0 - halfT, z1 - halfT}, {x1 + halfT, z1 + halfT});
          }
          if ((mask & world::WALL_WEST) != 0) {
            pushOutFromAABB2D(current, radius, {x0 - halfT, z0 - halfT}, {x0 + halfT, z1 + halfT});
          }
          if ((mask & world::WALL_EAST) != 0) {
            pushOutFromAABB2D(current, radius, {x1 - halfT, z0 - halfT}, {x1 + halfT, z1 + halfT});
          }
        }
      }
    }

    return {current.x, toPos.y, current.y};
  }

} // namespace ailivingmaze::game
