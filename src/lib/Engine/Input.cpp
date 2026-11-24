#include "Engine/Input.hpp"

namespace ailivingmaze::engine {

  void Input::beginFrame() {
    state_.mouseDeltaX = 0.0F;
    state_.mouseDeltaY = 0.0F;
    state_.windowResized = false;
    state_.toggleCursorCapture = false;
    state_.toggleDebugOverlay = false;
    state_.requestNewShader = false;
    state_.toggleShaderMode = false;

    const bool *keys = SDL_GetKeyboardState(nullptr);
    if (keys) {
      state_.forward = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
      state_.backward = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
      state_.left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
      state_.right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
      state_.sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    }
  }

  void Input::processEvent(const SDL_Event &event, bool cursorCaptured) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        state_.quitRequested = true;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        state_.windowResized = true;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if (cursorCaptured) {
          state_.mouseDeltaX += event.motion.xrel;
          state_.mouseDeltaY += event.motion.yrel;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (!cursorCaptured && event.button.button == SDL_BUTTON_LEFT) {
          state_.toggleCursorCapture = true;
        }
        break;
      case SDL_EVENT_KEY_DOWN:
        if (!event.key.repeat) {
          switch (event.key.scancode) {
            case SDL_SCANCODE_ESCAPE:
              state_.toggleCursorCapture = true;
              break;
            case SDL_SCANCODE_F1:
              state_.toggleDebugOverlay = true;
              break;
            case SDL_SCANCODE_F2:
              state_.requestNewShader = true;
              break;
            case SDL_SCANCODE_F3:
              state_.toggleShaderMode = true;
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }

} // namespace ailivingmaze::engine
