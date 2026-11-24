#include "Engine/Window.hpp"

#include "Render/GLFunctions.hpp"

#include <GL/gl.h>

namespace ailivingmaze::engine {

  Window::~Window() { shutdown(); }

  bool Window::init(const WindowConfig &config, std::string &errorOut) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      errorOut = std::string("SDL_Init failed: ") + SDL_GetError();
      return false;
    }
    sdlInitialized_ = true;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL;
    if (config.resizable) {
      flags |= SDL_WINDOW_RESIZABLE;
    }

    window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
    if (!window_) {
      errorOut = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
      shutdown();
      return false;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
      errorOut = std::string("SDL_GL_CreateContext failed: ") + SDL_GetError();
      shutdown();
      return false;
    }

    if (!SDL_GL_MakeCurrent(window_, glContext_)) {
      errorOut = std::string("SDL_GL_MakeCurrent failed: ") + SDL_GetError();
      shutdown();
      return false;
    }

    SDL_GL_SetSwapInterval(config.vsync ? 1 : 0);

    if (!render::gl().load()) {
      errorOut = "Failed to load OpenGL 3.3 Core functions via SDL_GL_GetProcAddress";
      shutdown();
      return false;
    }

    updateDimensions();
    glViewport(0, 0, width_, height_);
    return true;
  }

  void Window::shutdown() {
    if (window_ && relativeMouse_) {
      SDL_SetWindowRelativeMouseMode(window_, false);
      relativeMouse_ = false;
    }
    if (glContext_) {
      SDL_GL_DestroyContext(glContext_);
      glContext_ = nullptr;
    }
    if (window_) {
      SDL_DestroyWindow(window_);
      window_ = nullptr;
    }
    if (sdlInitialized_) {
      SDL_Quit();
      sdlInitialized_ = false;
    }
  }

  void Window::swapBuffers() {
    if (window_) {
      SDL_GL_SwapWindow(window_);
    }
  }

  void Window::setRelativeMouseMode(bool enabled) {
    if (!window_) {
      return;
    }
    if (SDL_SetWindowRelativeMouseMode(window_, enabled)) {
      relativeMouse_ = enabled;
    }
  }

  void Window::updateDimensions() {
    if (!window_) {
      return;
    }
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    if (w > 0 && h > 0) {
      width_ = w;
      height_ = h;
    }
  }

  std::string Window::glRendererString() const {
    const auto *s = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    return s ? std::string(s) : "Unknown";
  }

  std::string Window::glVersionString() const {
    const auto *s = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    return s ? std::string(s) : "Unknown";
  }

} // namespace ailivingmaze::engine
