#include "AI/OpenAICompatibleProvider.hpp"

#include "Shader/ShaderValidator.hpp"

#include <chrono>
#include <cstdlib>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace ailivingmaze::ai {

  namespace {
    std::size_t curlWriteCallback(char *ptr, std::size_t size, std::size_t nmemb, void *userdata) {
      auto *out = static_cast<std::string *>(userdata);
      const std::size_t total = size * nmemb;
      out->append(ptr, total);
      return total;
    }
  } // namespace

  OpenAICompatibleProvider::OpenAICompatibleProvider(AIConfig config)
      : config_(std::move(config)) {}

  std::string OpenAICompatibleProvider::readApiKeyFromEnv() const {
    if (config_.apiKeyEnvVar.empty()) {
      return {};
    }
    const char *val = std::getenv(config_.apiKeyEnvVar.c_str());
    return val != nullptr ? std::string(val) : std::string{};
  }

  bool OpenAICompatibleProvider::isLocalEndpoint() const {
    return config_.endpoint.find("localhost") != std::string::npos ||
           config_.endpoint.find("127.0.0.1") != std::string::npos;
  }

  bool OpenAICompatibleProvider::isAvailable() const {
    if (!config_.enabled || config_.endpoint.empty() || config_.model.empty()) {
      return false;
    }
    return isLocalEndpoint() || !readApiKeyFromEnv().empty();
  }

  bool OpenAICompatibleProvider::extractGlslFromApiJson(const std::string &jsonBody,
                                                        std::string &glslOut,
                                                        std::string &errorOut) {
    try {
      const auto root = nlohmann::json::parse(jsonBody);
      if (root.contains("error")) {
        errorOut = "API error: " + root["error"].dump();
        return false;
      }
      if (!root.contains("choices") || !root["choices"].is_array() || root["choices"].empty()) {
        errorOut = "API response missing choices array";
        return false;
      }
      const auto &msg = root["choices"][0];
      if (!msg.contains("message") || !msg["message"].contains("content") ||
          !msg["message"]["content"].is_string()) {
        errorOut = "API response missing choices[0].message.content";
        return false;
      }
      const std::string raw = msg["message"]["content"].get<std::string>();
      glslOut = shader::ShaderValidator::stripMarkdownFences(raw);
      if (glslOut.empty()) {
        errorOut = "Extracted GLSL content is empty";
        return false;
      }
      return true;
    } catch (const std::exception &e) {
      errorOut = std::string("Failed to parse API JSON: ") + e.what();
      return false;
    }
  }

  AIShaderResponse OpenAICompatibleProvider::generateShader(const ShaderPromptContext &ctx) {
    const auto t0 = std::chrono::steady_clock::now();
    AIShaderResponse res{};
    res.sectorId = ctx.theme.sectorId;
    res.promptUsed = buildStructuredShaderPrompt(ctx);
    res.modelUsed = config_.model;

    if (!isAvailable()) {
      res.errorMessage = "OpenAI-compatible provider unavailable (missing env var " +
                         config_.apiKeyEnvVar + " or disabled)";
      return res;
    }

    nlohmann::json payload;
    payload["model"] = config_.model;
    payload["temperature"] = 0.75;
    payload["max_tokens"] = 1400;
    payload["messages"] = nlohmann::json::array({
        {{"role", "system"},
         {"content",
          "You are a GLSL 330 procedural graphics shader author. Output only valid GLSL code "
          "implementing SurfaceOutput evaluateSurface(SurfaceInput s) without #version, uniforms, "
          "or main()."}},
        {{"role", "user"}, {"content", res.promptUsed}},
    });
    const std::string bodyStr = payload.dump();
    const std::string apiKey = readApiKeyFromEnv();

    const int attempts = std::max(1, config_.maxRetries + 1);
    for (int attempt = 0; attempt < attempts; ++attempt) {
      CURL *curl = curl_easy_init();
      if (!curl) {
        res.errorMessage = "curl_easy_init() failed";
        break;
      }

      std::string responseBody;
      struct curl_slist *headers = nullptr;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      if (!apiKey.empty()) {
        const std::string auth = "Authorization: Bearer " + apiKey;
        headers = curl_slist_append(headers, auth.c_str());
      }

      curl_easy_setopt(curl, CURLOPT_URL, config_.endpoint.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(bodyStr.size()));
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(config_.timeoutSeconds));
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

      const CURLcode code = curl_easy_perform(curl);
      long httpStatus = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);

      if (code != CURLE_OK) {
        res.errorMessage = std::string("HTTP request failed: ") + curl_easy_strerror(code);
        continue;
      }
      if (httpStatus < 200 || httpStatus >= 300) {
        res.errorMessage = "HTTP status " + std::to_string(httpStatus) + ": " +
                           responseBody.substr(0, 240);
        continue;
      }

      std::string glsl;
      std::string parseErr;
      if (extractGlslFromApiJson(responseBody, glsl, parseErr)) {
        res.glslCode = std::move(glsl);
        res.success = true;
        res.errorMessage.clear();
        break;
      }
      res.errorMessage = parseErr;
    }

    const auto t1 = std::chrono::steady_clock::now();
    res.generationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
  }

} // namespace ailivingmaze::ai
