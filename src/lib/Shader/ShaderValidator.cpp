#include "Shader/ShaderValidator.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <regex>

namespace ailivingmaze::shader {

  namespace {
    std::string trim(std::string_view sv) {
      std::size_t start = 0;
      while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start])) != 0) {
        ++start;
      }
      std::size_t end = sv.size();
      while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1])) != 0) {
        --end;
      }
      return std::string(sv.substr(start, end - start));
    }

    std::string stripComments(std::string_view code) {
      std::string out;
      out.reserve(code.size());
      bool inLine = false;
      bool inBlock = false;
      for (std::size_t i = 0; i < code.size(); ++i) {
        if (inLine) {
          if (code[i] == '\n') {
            inLine = false;
            out.push_back('\n');
          }
          continue;
        }
        if (inBlock) {
          if (code[i] == '*' && i + 1 < code.size() && code[i + 1] == '/') {
            inBlock = false;
            ++i;
          }
          continue;
        }
        if (code[i] == '/' && i + 1 < code.size() && code[i + 1] == '/') {
          inLine = true;
          ++i;
          continue;
        }
        if (code[i] == '/' && i + 1 < code.size() && code[i + 1] == '*') {
          inBlock = true;
          ++i;
          continue;
        }
        out.push_back(code[i]);
      }
      return out;
    }
  } // namespace

  std::string ShaderValidator::stripMarkdownFences(std::string_view rawInput) {
    std::string text = trim(rawInput);
    const auto firstFence = text.find("```");
    if (firstFence != std::string::npos) {
      auto contentStart = text.find('\n', firstFence);
      if (contentStart != std::string::npos) {
        ++contentStart;
        const auto closingFence = text.find("```", contentStart);
        if (closingFence != std::string::npos) {
          return trim(std::string_view(text).substr(contentStart, closingFence - contentStart));
        }
      }
    }
    return text;
  }

  ValidationResult ShaderValidator::validate(std::string_view rawInput) {
    ValidationResult res{};
    res.sanitizedCode = stripMarkdownFences(rawInput);

    if (res.sanitizedCode.size() < kMinLength) {
      res.errorMessage = "Shader code is empty or too short.";
      return res;
    }
    if (res.sanitizedCode.size() > kMaxLength) {
      res.errorMessage = "Shader code exceeds maximum allowed length (16 KB).";
      return res;
    }

    const std::string noComments = stripComments(res.sanitizedCode);

    // Check balanced braces and parentheses
    int braceDepth = 0;
    int parenDepth = 0;
    for (char c : noComments) {
      if (c == '{') {
        ++braceDepth;
      } else if (c == '}') {
        --braceDepth;
      } else if (c == '(') {
        ++parenDepth;
      } else if (c == ')') {
        --parenDepth;
      }
      if (braceDepth < 0 || parenDepth < 0) {
        res.errorMessage = "Unbalanced braces or parentheses in GLSL.";
        return res;
      }
    }
    if (braceDepth != 0 || parenDepth != 0) {
      res.errorMessage = "Unclosed braces or parentheses in GLSL.";
      return res;
    }

    // Forbidden directives / pipeline-breaking constructs
    constexpr std::array<std::pair<const char *, const char *>, 11> kForbidden{{
        {"#version", "Directive #version is managed by the engine template."},
        {"#extension", "Directive #extension is not permitted."},
        {"#include", "Directive #include is not permitted."},
        {"layout(", "layout() qualifiers are managed by the engine template."},
        {"uniform ", "Declaring custom uniforms is not permitted."},
        {"void main", "Entry point main() is managed by the engine template."},
        {"discard;", "discard is not permitted in surface shaders."},
        {"gl_FragDepth", "Writing gl_FragDepth is not permitted."},
        {"imageStore", "Image store operations are not permitted."},
        {"while(", "Unbounded while loops are not permitted."},
        {"while (", "Unbounded while loops are not permitted."},
    }};

    for (const auto &[token, reason] : kForbidden) {
      if (noComments.find(token) != std::string::npos) {
        res.errorMessage = std::string("Forbidden GLSL construct: ") + reason;
        return res;
      }
    }

    // Check required entry function signature
    static const std::regex entryRegex(R"(SurfaceOutput\s+evaluateSurface\s*\(\s*SurfaceInput\s+[A-Za-z_][A-Za-z0-9_]*\s*\))");
    if (!std::regex_search(noComments, entryRegex)) {
      res.errorMessage = "Missing required function: SurfaceOutput evaluateSurface(SurfaceInput s)";
      return res;
    }

    // Limit for-loop count and literal iteration bounds to prevent GPU hangs
    static const std::regex forRegex(R"(\bfor\s*\(([^;]*);([^;]*);([^\)]*)\))");
    auto it = std::sregex_iterator(noComments.begin(), noComments.end(), forRegex);
    auto end = std::sregex_iterator();
    int loopCount = 0;
    for (; it != end; ++it) {
      ++loopCount;
      if (loopCount > kMaxForLoops) {
        res.errorMessage = "Shader exceeds maximum allowed number of for-loops (" +
                           std::to_string(kMaxForLoops) + ").";
        return res;
      }
      const std::string cond = (*it)[2].str();
      static const std::regex numRegex(R"(\b([0-9]+)\b)");
      auto nit = std::sregex_iterator(cond.begin(), cond.end(), numRegex);
      for (; nit != end; ++nit) {
        const int bound = std::stoi((*nit)[1].str());
        if (bound > kMaxLoopIterations) {
          res.errorMessage = "For-loop iteration bound (" + std::to_string(bound) +
                             ") exceeds limit (" + std::to_string(kMaxLoopIterations) + ").";
          return res;
        }
      }
    }

    res.valid = true;
    return res;
  }

} // namespace ailivingmaze::shader
