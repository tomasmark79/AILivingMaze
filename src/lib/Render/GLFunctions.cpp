#include "Render/GLFunctions.hpp"

#include <SDL3/SDL.h>

namespace ailivingmaze::render {

  namespace {
    template <typename Fn> bool loadProc(Fn &fn, const char *name) {
      fn = reinterpret_cast<Fn>(SDL_GL_GetProcAddress(name));
      return fn != nullptr;
    }
  } // namespace

  bool GLFunctions::load() {
    if (loaded) {
      return true;
    }

    bool ok = true;
    ok &= loadProc(GenVertexArrays, "glGenVertexArrays");
    ok &= loadProc(DeleteVertexArrays, "glDeleteVertexArrays");
    ok &= loadProc(BindVertexArray, "glBindVertexArray");

    ok &= loadProc(GenBuffers, "glGenBuffers");
    ok &= loadProc(DeleteBuffers, "glDeleteBuffers");
    ok &= loadProc(BindBuffer, "glBindBuffer");
    ok &= loadProc(BufferData, "glBufferData");
    ok &= loadProc(BufferSubData, "glBufferSubData");

    ok &= loadProc(EnableVertexAttribArray, "glEnableVertexAttribArray");
    ok &= loadProc(VertexAttribPointer, "glVertexAttribPointer");

    ok &= loadProc(CreateShader, "glCreateShader");
    ok &= loadProc(DeleteShader, "glDeleteShader");
    ok &= loadProc(ShaderSource, "glShaderSource");
    ok &= loadProc(CompileShader, "glCompileShader");
    ok &= loadProc(GetShaderiv, "glGetShaderiv");
    ok &= loadProc(GetShaderInfoLog, "glGetShaderInfoLog");

    ok &= loadProc(CreateProgram, "glCreateProgram");
    ok &= loadProc(DeleteProgram, "glDeleteProgram");
    ok &= loadProc(AttachShader, "glAttachShader");
    ok &= loadProc(DetachShader, "glDetachShader");
    ok &= loadProc(LinkProgram, "glLinkProgram");
    ok &= loadProc(GetProgramiv, "glGetProgramiv");
    ok &= loadProc(GetProgramInfoLog, "glGetProgramInfoLog");
    ok &= loadProc(UseProgram, "glUseProgram");

    ok &= loadProc(GetUniformLocation, "glGetUniformLocation");
    ok &= loadProc(Uniform1i, "glUniform1i");
    ok &= loadProc(Uniform1f, "glUniform1f");
    ok &= loadProc(Uniform2f, "glUniform2f");
    ok &= loadProc(Uniform3f, "glUniform3f");
    ok &= loadProc(Uniform4f, "glUniform4f");
    ok &= loadProc(UniformMatrix4fv, "glUniformMatrix4fv");

    ok &= loadProc(ActiveTexture, "glActiveTexture");
    ok &= loadProc(GenFramebuffers, "glGenFramebuffers");
    ok &= loadProc(DeleteFramebuffers, "glDeleteFramebuffers");
    ok &= loadProc(BindFramebuffer, "glBindFramebuffer");
    ok &= loadProc(FramebufferTexture2D, "glFramebufferTexture2D");
    ok &= loadProc(CheckFramebufferStatus, "glCheckFramebufferStatus");

    loaded = ok;
    return loaded;
  }

  GLFunctions &gl() {
    static GLFunctions instance;
    return instance;
  }

} // namespace ailivingmaze::render
