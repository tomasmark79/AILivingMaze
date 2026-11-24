#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <cstddef>
#include <cstdint>

// Minimal OpenGL 3.3 Core function pointers loaded via SDL_GL_GetProcAddress.
// Avoids external GL loader dependencies while keeping full Linux/NixOS compatibility.

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif

#ifndef __EMSCRIPTEN__
using GLchar = char;
using GLsizeiptr = std::ptrdiff_t;
using GLintptr = std::ptrdiff_t;
#endif

namespace ailivingmaze::render {

  struct GLFunctions {
    // Buffers & VAOs
    void (*GenVertexArrays)(GLsizei, GLuint *) = nullptr;
    void (*DeleteVertexArrays)(GLsizei, const GLuint *) = nullptr;
    void (*BindVertexArray)(GLuint) = nullptr;

    void (*GenBuffers)(GLsizei, GLuint *) = nullptr;
    void (*DeleteBuffers)(GLsizei, const GLuint *) = nullptr;
    void (*BindBuffer)(GLenum, GLuint) = nullptr;
    void (*BufferData)(GLenum, GLsizeiptr, const void *, GLenum) = nullptr;
    void (*BufferSubData)(GLenum, GLintptr, GLsizeiptr, const void *) = nullptr;

    void (*EnableVertexAttribArray)(GLuint) = nullptr;
    void (*VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *) = nullptr;

    // Shaders & Programs
    GLuint (*CreateShader)(GLenum) = nullptr;
    void (*DeleteShader)(GLuint) = nullptr;
    void (*ShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *) = nullptr;
    void (*CompileShader)(GLuint) = nullptr;
    void (*GetShaderiv)(GLuint, GLenum, GLint *) = nullptr;
    void (*GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = nullptr;

    GLuint (*CreateProgram)() = nullptr;
    void (*DeleteProgram)(GLuint) = nullptr;
    void (*AttachShader)(GLuint, GLuint) = nullptr;
    void (*DetachShader)(GLuint, GLuint) = nullptr;
    void (*LinkProgram)(GLuint) = nullptr;
    void (*GetProgramiv)(GLuint, GLenum, GLint *) = nullptr;
    void (*GetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = nullptr;
    void (*UseProgram)(GLuint) = nullptr;

    // Uniforms
    GLint (*GetUniformLocation)(GLuint, const GLchar *) = nullptr;
    void (*Uniform1i)(GLint, GLint) = nullptr;
    void (*Uniform1f)(GLint, GLfloat) = nullptr;
    void (*Uniform2f)(GLint, GLfloat, GLfloat) = nullptr;
    void (*Uniform3f)(GLint, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*Uniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*UniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *) = nullptr;

    // Textures & Framebuffers
    void (*ActiveTexture)(GLenum) = nullptr;
    void (*GenFramebuffers)(GLsizei, GLuint *) = nullptr;
    void (*DeleteFramebuffers)(GLsizei, const GLuint *) = nullptr;
    void (*BindFramebuffer)(GLenum, GLuint) = nullptr;
    void (*FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
    GLenum (*CheckFramebufferStatus)(GLenum) = nullptr;

    bool loaded = false;

    bool load();
  };

  GLFunctions &gl();

} // namespace ailivingmaze::render
