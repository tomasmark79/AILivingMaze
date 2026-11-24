#pragma once

#include <string>
#include <string_view>

namespace ailivingmaze::shader {

  struct ValidationResult {
    bool valid = false;
    std::string errorMessage;
    std::string sanitizedCode;
  };

  class ShaderValidator {
  public:
    static constexpr std::size_t kMinLength = 30;
    static constexpr std::size_t kMaxLength = 16384;
    static constexpr int kMaxForLoops = 4;
    static constexpr int kMaxLoopIterations = 32;

    // Extracts GLSL from optional markdown fences and validates structural safety.
    [[nodiscard]] static ValidationResult validate(std::string_view rawInput);

    [[nodiscard]] static std::string stripMarkdownFences(std::string_view rawInput);
  };

} // namespace ailivingmaze::shader
