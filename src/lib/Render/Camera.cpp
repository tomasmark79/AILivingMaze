#include "Render/Camera.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace ailivingmaze::render {

  bool Frustum::intersectsAABB(const glm::vec3 &minBounds,
                               const glm::vec3 &maxBounds) const noexcept {
    for (const auto &p : planes) {
      glm::vec3 positiveVertex(p.x >= 0.0F ? maxBounds.x : minBounds.x,
                               p.y >= 0.0F ? maxBounds.y : minBounds.y,
                               p.z >= 0.0F ? maxBounds.z : minBounds.z);
      if (glm::dot(glm::vec3(p), positiveVertex) + p.w < 0.0F) {
        return false;
      }
    }
    return true;
  }

  void Camera::setAngles(float yawDeg, float pitchDeg) noexcept {
    yaw_ = yawDeg;
    pitch_ = std::clamp(pitchDeg, -85.0F, 85.0F);
  }

  void Camera::addLookDelta(float deltaX, float deltaY, float sensitivity) noexcept {
    yaw_ += deltaX * sensitivity;
    const float pitchDelta = invertY_ ? (deltaY * sensitivity) : (-deltaY * sensitivity);
    pitch_ = std::clamp(pitch_ + pitchDelta, -85.0F, 85.0F);
  }

  glm::vec3 Camera::forward() const noexcept {
    const float yawRad = glm::radians(yaw_);
    const float pitchRad = glm::radians(pitch_);
    glm::vec3 dir{std::cos(yawRad) * std::cos(pitchRad), std::sin(pitchRad),
                  std::sin(yawRad) * std::cos(pitchRad)};
    return glm::normalize(dir);
  }

  glm::vec3 Camera::forwardXZ() const noexcept {
    const float yawRad = glm::radians(yaw_);
    return glm::normalize(glm::vec3{std::cos(yawRad), 0.0F, std::sin(yawRad)});
  }

  glm::vec3 Camera::rightXZ() const noexcept {
    const glm::vec3 fwd = forwardXZ();
    return glm::normalize(glm::cross(fwd, glm::vec3{0.0F, 1.0F, 0.0F}));
  }

  glm::mat4 Camera::viewMatrix() const noexcept {
    const glm::vec3 fwd = forward();
    return glm::lookAt(position_, position_ + fwd, glm::vec3{0.0F, 1.0F, 0.0F});
  }

  glm::mat4 Camera::projectionMatrix(float aspectRatio, float fovDeg, float nearPlane,
                                     float farPlane) const noexcept {
    return glm::perspective(glm::radians(fovDeg), aspectRatio, nearPlane, farPlane);
  }

  Frustum Camera::extractFrustum(const glm::mat4 &vp) const noexcept {
    Frustum f;
    // Column-major GLM matrix: row i is (vp[0][i], vp[1][i], vp[2][i], vp[3][i])
    auto row = [&](int i) { return glm::vec4(vp[0][i], vp[1][i], vp[2][i], vp[3][i]); };
    const glm::vec4 r0 = row(0);
    const glm::vec4 r1 = row(1);
    const glm::vec4 r2 = row(2);
    const glm::vec4 r3 = row(3);

    f.planes[0] = r3 + r0; // Left
    f.planes[1] = r3 - r0; // Right
    f.planes[2] = r3 + r1; // Bottom
    f.planes[3] = r3 - r1; // Top
    f.planes[4] = r3 + r2; // Near
    f.planes[5] = r3 - r2; // Far

    for (auto &plane : f.planes) {
      const float len = glm::length(glm::vec3(plane));
      if (len > 1e-6F) {
        plane /= len;
      }
    }
    return f;
  }

} // namespace ailivingmaze::render
