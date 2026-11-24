#pragma once

#include <SDL3/SDL.h>
#include <string>

namespace ailivingmaze::engine {

  struct WindowConfig {
    std::string title = "AI Living Maze";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool resizable = true;
  };

  class Window {
  public:
    Window() = default;
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;

    bool init(const WindowConfig &config, std::string &errorOut);
    void shutdown();

    void swapBuffers();
    void setRelativeMouseMode(bool enabled);
    [[nodiscard]] bool isRelativeMouseMode() const noexcept { return relativeMouse_; }

    void updateDimensions();
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] float aspectRatio() const noexcept {
      return height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 16.0F / 9.0F;
    }
    [[nodiscard]] SDL_Window *handle() const noexcept { return window_; }
    [[nodiscard]] bool isInitialized() const noexcept { return window_ != nullptr && glContext_ != nullptr; }

    [[nodiscard]] std::string glRendererString() const;
    [[nodiscard]] std::string glVersionString() const;

  private:
    SDL_Window *window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    bool sdlInitialized_ = false;
    bool relativeMouse_ = false;
    int width_ = 1280;
    int height_ = 720;
  };

} // namespace ailivingmaze::engine
