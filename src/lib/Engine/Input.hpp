#pragma once

#include <SDL3/SDL.h>

namespace ailivingmaze::engine {

  struct InputState {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool sprint = false;

    float mouseDeltaX = 0.0F;
    float mouseDeltaY = 0.0F;

    bool quitRequested = false;
    bool windowResized = false;
    bool toggleCursorCapture = false;
    bool toggleDebugOverlay = false;
    bool requestNewShader = false;
    bool toggleShaderMode = false;
  };

  class Input {
  public:
    void beginFrame();
    void processEvent(const SDL_Event &event, bool cursorCaptured);
    [[nodiscard]] const InputState &state() const noexcept { return state_; }

  private:
    InputState state_{};
  };

} // namespace ailivingmaze::engine
