#pragma once

#include <array>
#include <glm/glm.hpp>

namespace ailivingmaze::render {

  struct Frustum {
    // Planes in ax + by + cz + d >= 0 form (left, right, bottom, top, near, far)
    std::array<glm::vec4, 6> planes{};

    [[nodiscard]] bool intersectsAABB(const glm::vec3 &minBounds,
                                      const glm::vec3 &maxBounds) const noexcept;
  };

  class Camera {
  public:
    void setPosition(const glm::vec3 &pos) noexcept { position_ = pos; }
    void setAngles(float yawDeg, float pitchDeg) noexcept;
    void addLookDelta(float deltaX, float deltaY, float sensitivity = 0.12F) noexcept;
    void setInvertY(bool invert) noexcept { invertY_ = invert; }
    void toggleInvertY() noexcept { invertY_ = !invertY_; }

    [[nodiscard]] const glm::vec3 &position() const noexcept { return position_; }
    [[nodiscard]] float yaw() const noexcept { return yaw_; }
    [[nodiscard]] float pitch() const noexcept { return pitch_; }
    [[nodiscard]] bool isInvertY() const noexcept { return invertY_; }

    [[nodiscard]] glm::vec3 forward() const noexcept;
    [[nodiscard]] glm::vec3 forwardXZ() const noexcept;
    [[nodiscard]] glm::vec3 rightXZ() const noexcept;

    [[nodiscard]] glm::mat4 viewMatrix() const noexcept;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio, float fovDeg = 75.0F,
                                             float nearPlane = 0.1F,
                                             float farPlane = 260.0F) const noexcept;
    [[nodiscard]] Frustum extractFrustum(const glm::mat4 &viewProj) const noexcept;

  private:
    glm::vec3 position_{0.0F, 1.7F, 0.0F};
    float yaw_ = -90.0F;
    float pitch_ = 0.0F;
    bool invertY_ = false;
  };

} // namespace ailivingmaze::render
