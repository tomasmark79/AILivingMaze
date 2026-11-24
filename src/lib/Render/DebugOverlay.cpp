#include "Render/DebugOverlay.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace ailivingmaze::render {

  namespace {
    // Compact 5x7 ASCII bitmap font (codes 32..126) for zero-dependency HUD rendering.
    // Each character has 5 columns of 7 bits (LSB = top row).
    struct Glyph5x7 {
      uint8_t cols[5];
    };

    constexpr std::array<Glyph5x7, 95> kFont5x7{{
        {{0x00, 0x00, 0x00, 0x00, 0x00}}, // ' '
        {{0x00, 0x00, 0x5F, 0x00, 0x00}}, // '!'
        {{0x00, 0x07, 0x00, 0x07, 0x00}}, // '"'
        {{0x14, 0x7F, 0x14, 0x7F, 0x14}}, // '#'
        {{0x24, 0x2A, 0x7F, 0x2A, 0x12}}, // '$'
        {{0x23, 0x13, 0x08, 0x64, 0x62}}, // '%'
        {{0x36, 0x49, 0x55, 0x22, 0x50}}, // '&'
        {{0x00, 0x05, 0x03, 0x00, 0x00}}, // '''
        {{0x00, 0x1C, 0x22, 0x41, 0x00}}, // '('
        {{0x00, 0x41, 0x22, 0x1C, 0x00}}, // ')'
        {{0x14, 0x08, 0x3E, 0x08, 0x14}}, // '*'
        {{0x08, 0x08, 0x3E, 0x08, 0x08}}, // '+'
        {{0x00, 0x50, 0x30, 0x00, 0x00}}, // ','
        {{0x08, 0x08, 0x08, 0x08, 0x08}}, // '-'
        {{0x00, 0x60, 0x60, 0x00, 0x00}}, // '.'
        {{0x20, 0x10, 0x08, 0x04, 0x02}}, // '/'
        {{0x3E, 0x51, 0x49, 0x45, 0x3E}}, // '0'
        {{0x00, 0x42, 0x7F, 0x40, 0x00}}, // '1'
        {{0x42, 0x61, 0x51, 0x49, 0x46}}, // '2'
        {{0x21, 0x41, 0x45, 0x4B, 0x31}}, // '3'
        {{0x18, 0x14, 0x12, 0x7F, 0x10}}, // '4'
        {{0x27, 0x45, 0x45, 0x45, 0x39}}, // '5'
        {{0x3C, 0x4A, 0x49, 0x49, 0x30}}, // '6'
        {{0x01, 0x71, 0x09, 0x05, 0x03}}, // '7'
        {{0x36, 0x49, 0x49, 0x49, 0x36}}, // '8'
        {{0x06, 0x49, 0x49, 0x29, 0x1E}}, // '9'
        {{0x00, 0x36, 0x36, 0x00, 0x00}}, // ':'
        {{0x00, 0x56, 0x36, 0x00, 0x00}}, // ';'
        {{0x08, 0x14, 0x22, 0x41, 0x00}}, // '<'
        {{0x14, 0x14, 0x14, 0x14, 0x14}}, // '='
        {{0x00, 0x41, 0x22, 0x14, 0x08}}, // '>'
        {{0x02, 0x01, 0x51, 0x09, 0x06}}, // '?'
        {{0x32, 0x49, 0x79, 0x41, 0x3E}}, // '@'
        {{0x7E, 0x11, 0x11, 0x11, 0x7E}}, // 'A'
        {{0x7F, 0x49, 0x49, 0x49, 0x36}}, // 'B'
        {{0x3E, 0x41, 0x41, 0x41, 0x22}}, // 'C'
        {{0x7F, 0x41, 0x41, 0x22, 0x1C}}, // 'D'
        {{0x7F, 0x49, 0x49, 0x49, 0x41}}, // 'E'
        {{0x7F, 0x09, 0x09, 0x09, 0x01}}, // 'F'
        {{0x3E, 0x41, 0x49, 0x49, 0x7A}}, // 'G'
        {{0x7F, 0x08, 0x08, 0x08, 0x7F}}, // 'H'
        {{0x00, 0x41, 0x7F, 0x41, 0x00}}, // 'I'
        {{0x20, 0x40, 0x41, 0x3F, 0x01}}, // 'J'
        {{0x7F, 0x08, 0x14, 0x22, 0x41}}, // 'K'
        {{0x7F, 0x40, 0x40, 0x40, 0x40}}, // 'L'
        {{0x7F, 0x02, 0x0C, 0x02, 0x7F}}, // 'M'
        {{0x7F, 0x04, 0x08, 0x10, 0x7F}}, // 'N'
        {{0x3E, 0x41, 0x41, 0x41, 0x3E}}, // 'O'
        {{0x7F, 0x09, 0x09, 0x09, 0x06}}, // 'P'
        {{0x3E, 0x41, 0x51, 0x21, 0x5E}}, // 'Q'
        {{0x7F, 0x09, 0x19, 0x29, 0x46}}, // 'R'
        {{0x46, 0x49, 0x49, 0x49, 0x31}}, // 'S'
        {{0x01, 0x01, 0x7F, 0x01, 0x01}}, // 'T'
        {{0x3F, 0x40, 0x40, 0x40, 0x3F}}, // 'U'
        {{0x1F, 0x20, 0x40, 0x20, 0x1F}}, // 'V'
        {{0x3F, 0x40, 0x38, 0x40, 0x3F}}, // 'W'
        {{0x63, 0x14, 0x08, 0x14, 0x63}}, // 'X'
        {{0x07, 0x08, 0x70, 0x08, 0x07}}, // 'Y'
        {{0x61, 0x51, 0x49, 0x45, 0x43}}, // 'Z'
        {{0x00, 0x7F, 0x41, 0x41, 0x00}}, // '['
        {{0x02, 0x04, 0x08, 0x10, 0x20}}, // '\'
        {{0x00, 0x41, 0x41, 0x7F, 0x00}}, // ']'
        {{0x04, 0x02, 0x01, 0x02, 0x04}}, // '^'
        {{0x40, 0x40, 0x40, 0x40, 0x40}}, // '_'
        {{0x00, 0x01, 0x02, 0x04, 0x00}}, // '`'
        {{0x20, 0x54, 0x54, 0x54, 0x78}}, // 'a'
        {{0x7F, 0x48, 0x44, 0x44, 0x38}}, // 'b'
        {{0x38, 0x44, 0x44, 0x44, 0x20}}, // 'c'
        {{0x38, 0x44, 0x44, 0x48, 0x7F}}, // 'd'
        {{0x38, 0x54, 0x54, 0x54, 0x18}}, // 'e'
        {{0x08, 0x7E, 0x09, 0x01, 0x02}}, // 'f'
        {{0x0C, 0x52, 0x52, 0x52, 0x3E}}, // 'g'
        {{0x7F, 0x08, 0x04, 0x04, 0x78}}, // 'h'
        {{0x00, 0x44, 0x7D, 0x40, 0x00}}, // 'i'
        {{0x20, 0x40, 0x44, 0x3D, 0x00}}, // 'j'
        {{0x7F, 0x10, 0x28, 0x44, 0x00}}, // 'k'
        {{0x00, 0x41, 0x7F, 0x40, 0x00}}, // 'l'
        {{0x7C, 0x04, 0x18, 0x04, 0x78}}, // 'm'
        {{0x7C, 0x08, 0x04, 0x04, 0x78}}, // 'n'
        {{0x38, 0x44, 0x44, 0x44, 0x38}}, // 'o'
        {{0x7C, 0x14, 0x14, 0x14, 0x08}}, // 'p'
        {{0x08, 0x14, 0x14, 0x18, 0x7C}}, // 'q'
        {{0x7C, 0x08, 0x04, 0x04, 0x08}}, // 'r'
        {{0x48, 0x54, 0x54, 0x54, 0x20}}, // 's'
        {{0x04, 0x3F, 0x44, 0x40, 0x20}}, // 't'
        {{0x3C, 0x40, 0x40, 0x20, 0x7C}}, // 'u'
        {{0x1C, 0x20, 0x40, 0x20, 0x1C}}, // 'v'
        {{0x3C, 0x40, 0x30, 0x40, 0x3C}}, // 'w'
        {{0x44, 0x28, 0x10, 0x28, 0x44}}, // 'x'
        {{0x0C, 0x50, 0x50, 0x50, 0x3C}}, // 'y'
        {{0x44, 0x64, 0x54, 0x4C, 0x44}}, // 'z'
        {{0x00, 0x08, 0x36, 0x41, 0x00}}, // '{'
        {{0x00, 0x00, 0x7F, 0x00, 0x00}}, // '|'
        {{0x00, 0x41, 0x36, 0x08, 0x00}}, // '}'
        {{0x10, 0x08, 0x08, 0x10, 0x08}}, // '~'
    }};

    constexpr const char *kOverlayVs = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
layout(location = 3) in float a_textured;

uniform vec2 u_screenSize;

out vec2 v_uv;
out vec4 v_color;
out float v_textured;

void main() {
  vec2 ndc = (a_pos / u_screenSize) * 2.0 - 1.0;
  ndc.y = -ndc.y;
  gl_Position = vec4(ndc, 0.0, 1.0);
  v_uv = a_uv;
  v_color = a_color;
  v_textured = a_textured;
}
)GLSL";

    constexpr const char *kOverlayFs = R"GLSL(
#version 330 core
in vec2 v_uv;
in vec4 v_color;
in float v_textured;

uniform sampler2D u_fontTex;
out vec4 FragColor;

void main() {
  float alpha = v_color.a;
  if (v_textured > 0.5) {
    alpha *= texture(u_fontTex, v_uv).r;
  }
  if (alpha < 0.01) {
    discard;
  }
  FragColor = vec4(v_color.rgb, alpha);
}
)GLSL";

    constexpr int kAtlasCols = 16;
    constexpr int kAtlasRows = 6;
    constexpr int kCellW = 6;
    constexpr int kCellH = 8;
    constexpr int kAtlasW = kAtlasCols * kCellW;
    constexpr int kAtlasH = kAtlasRows * kCellH;
  } // namespace

  DebugOverlay::~DebugOverlay() { shutdown(); }

  bool DebugOverlay::init(std::string &errorOut) {
    auto &g = gl();
    if (!g.loaded) {
      errorOut = "OpenGL not loaded for DebugOverlay";
      return false;
    }

    auto compile = [&](GLenum type, const char *src, GLuint &out) {
      out = g.CreateShader(type);
      g.ShaderSource(out, 1, &src, nullptr);
      g.CompileShader(out);
      GLint ok = GL_FALSE;
      g.GetShaderiv(out, GL_COMPILE_STATUS, &ok);
      if (ok == GL_FALSE) {
        g.DeleteShader(out);
        out = 0;
        return false;
      }
      return true;
    };

    GLuint vs = 0;
    GLuint fs = 0;
    if (!compile(GL_VERTEX_SHADER, kOverlayVs, vs) ||
        !compile(GL_FRAGMENT_SHADER, kOverlayFs, fs)) {
      if (vs != 0) {
        g.DeleteShader(vs);
      }
      errorOut = "Failed to compile DebugOverlay shaders";
      return false;
    }

    program_ = g.CreateProgram();
    g.AttachShader(program_, vs);
    g.AttachShader(program_, fs);
    g.LinkProgram(program_);
    g.DetachShader(program_, vs);
    g.DetachShader(program_, fs);
    g.DeleteShader(vs);
    g.DeleteShader(fs);

    // Build 1-channel bitmap font atlas
    std::vector<uint8_t> pixels(static_cast<std::size_t>(kAtlasW * kAtlasH), 0);
    for (int idx = 0; idx < static_cast<int>(kFont5x7.size()); ++idx) {
      const int gx = (idx % kAtlasCols) * kCellW;
      const int gy = (idx / kAtlasCols) * kCellH;
      const auto &glyph = kFont5x7[static_cast<std::size_t>(idx)];
      for (int col = 0; col < 5; ++col) {
        for (int row = 0; row < 7; ++row) {
          if (((glyph.cols[col] >> row) & 1U) != 0U) {
            pixels[static_cast<std::size_t>((gy + row) * kAtlasW + (gx + col))] = 255;
          }
        }
      }
    }

    glGenTextures(1, &fontTex_);
    glBindTexture(GL_TEXTURE_2D, fontTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAtlasW, kAtlasH, 0, GL_RED, GL_UNSIGNED_BYTE,
                 pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    g.GenVertexArrays(1, &vao_);
    g.GenBuffers(1, &vbo_);
    g.BindVertexArray(vao_);
    g.BindBuffer(GL_ARRAY_BUFFER, vbo_);
    const GLsizei stride = static_cast<GLsizei>(sizeof(QuadVertex));
    g.EnableVertexAttribArray(0);
    g.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(QuadVertex, x)));
    g.EnableVertexAttribArray(1);
    g.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(QuadVertex, u)));
    g.EnableVertexAttribArray(2);
    g.VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(QuadVertex, r)));
    g.EnableVertexAttribArray(3);
    g.VertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(QuadVertex, textured)));
    g.BindVertexArray(0);
    return true;
  }

  void DebugOverlay::shutdown() {
    auto &g = gl();
    if (!g.loaded) {
      return;
    }
    if (vbo_ != 0) {
      g.DeleteBuffers(1, &vbo_);
      vbo_ = 0;
    }
    if (vao_ != 0) {
      g.DeleteVertexArrays(1, &vao_);
      vao_ = 0;
    }
    if (program_ != 0) {
      g.DeleteProgram(program_);
      program_ = 0;
    }
    if (fontTex_ != 0) {
      glDeleteTextures(1, &fontTex_);
      fontTex_ = 0;
    }
  }

  void DebugOverlay::appendRect(std::vector<QuadVertex> &verts, float x, float y, float w, float h,
                                const glm::vec4 &c) {
    const QuadVertex v0{x, y, 0, 0, c.r, c.g, c.b, c.a, 0.0F};
    const QuadVertex v1{x + w, y, 0, 0, c.r, c.g, c.b, c.a, 0.0F};
    const QuadVertex v2{x + w, y + h, 0, 0, c.r, c.g, c.b, c.a, 0.0F};
    const QuadVertex v3{x, y + h, 0, 0, c.r, c.g, c.b, c.a, 0.0F};
    verts.push_back(v0);
    verts.push_back(v1);
    verts.push_back(v2);
    verts.push_back(v0);
    verts.push_back(v2);
    verts.push_back(v3);
  }

  void DebugOverlay::appendText(std::vector<QuadVertex> &verts, float x, float y, float scale,
                                const std::string &text, const glm::vec4 &c) {
    float cursorX = x;
    const float cw = static_cast<float>(kCellW) * scale;
    const float ch = static_cast<float>(kCellH) * scale;
    const float du = static_cast<float>(kCellW) / static_cast<float>(kAtlasW);
    const float dv = static_cast<float>(kCellH) / static_cast<float>(kAtlasH);

    for (char chRaw : text) {
      int code = static_cast<unsigned char>(chRaw);
      if (code < 32 || code > 126) {
        code = '?';
      }
      const int idx = code - 32;
      const float u0 = static_cast<float>(idx % kAtlasCols) * du;
      const float v0 = static_cast<float>(idx / kAtlasCols) * dv;
      const float u1 = u0 + du;
      const float v1 = v0 + dv;

      const QuadVertex q0{cursorX, y, u0, v0, c.r, c.g, c.b, c.a, 1.0F};
      const QuadVertex q1{cursorX + cw, y, u1, v0, c.r, c.g, c.b, c.a, 1.0F};
      const QuadVertex q2{cursorX + cw, y + ch, u1, v1, c.r, c.g, c.b, c.a, 1.0F};
      const QuadVertex q3{cursorX, y + ch, u0, v1, c.r, c.g, c.b, c.a, 1.0F};
      verts.push_back(q0);
      verts.push_back(q1);
      verts.push_back(q2);
      verts.push_back(q0);
      verts.push_back(q2);
      verts.push_back(q3);

      cursorX += cw;
    }
  }

  void DebugOverlay::render(int screenWidth, int screenHeight, const DebugOverlayData &d) {
    if (!visible_ || program_ == 0 || screenWidth <= 0 || screenHeight <= 0) {
      return;
    }

    auto fmt1 = [](double v) {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << v;
      return oss.str();
    };

    std::vector<std::string> lines;
    lines.push_back("AI LIVING MAZE [F1 HUD | F2 Shader | F3 Mode | F4 InvY | ESC Mouse]");
    lines.push_back("FPS: " + fmt1(d.fps) + "  (" + fmt1(d.frameTimeMs) + " ms)" +
                    (d.cursorCaptured ? "  [LOCKED]" : "  [CURSOR FREE - Click to lock]") +
                    (d.invertMouseY ? "  [InvY:ON]" : "  [InvY:OFF]"));
    lines.push_back("Pos: (" + fmt1(d.playerPos.x) + ", " + fmt1(d.playerPos.y) + ", " +
                    fmt1(d.playerPos.z) + ")  Chunk: (" + std::to_string(d.chunkX) + "," +
                    std::to_string(d.chunkZ) + ")");
    lines.push_back("Sector: " + d.sectorId + "  Style: " + d.sectorStyle);
    lines.push_back("Chunks Loaded: " + std::to_string(d.loadedChunks) +
                    "  Rendered: " + std::to_string(d.renderedChunks) +
                    "  DrawCalls: " + std::to_string(d.drawCalls));
    lines.push_back("Shaders Active: " + std::to_string(d.activeShaders) +
                    "  Mode: " + (d.usingAIShaders ? "AI-GENERATED" : "DEFAULT FALLBACK"));
    lines.push_back("AI Provider: " + d.aiProviderStatus +
                    "  Pending: " + std::to_string(d.pendingAIRequests));
    lines.push_back("Timing  Gen: " + fmt1(d.lastGenerationMs) + " ms  Compile: " +
                    fmt1(d.lastCompileMs) + " ms");
    lines.push_back("Cache   Hits: " + std::to_string(d.cacheHits) +
                    "  Misses: " + std::to_string(d.cacheMisses) +
                    "  Disk: " + std::to_string(d.cacheStored));
    if (!d.lastShaderError.empty()) {
      std::string err = d.lastShaderError;
      if (err.size() > 74) {
        err = err.substr(0, 71) + "...";
      }
      lines.push_back("Last Error: " + err);
    }

    const float scale = 2.0F;
    const float lineHeight = static_cast<float>(kCellH) * scale + 4.0F;
    const float pad = 12.0F;
    const float boxW = 860.0F;
    const float boxH = pad * 2.0F + static_cast<float>(lines.size()) * lineHeight;

    std::vector<QuadVertex> verts;
    verts.reserve(lines.size() * 80 * 6);
    appendRect(verts, 12.0F, 12.0F, boxW, boxH, {0.03F, 0.05F, 0.08F, 0.78F});

    for (std::size_t i = 0; i < lines.size(); ++i) {
      glm::vec4 color{0.88F, 0.94F, 0.98F, 1.0F};
      if (i == 0) {
        color = {0.35F, 0.90F, 1.0F, 1.0F};
      } else if (lines[i].rfind("Last Error:", 0) == 0) {
        color = {1.0F, 0.42F, 0.38F, 1.0F};
      }
      appendText(verts, 12.0F + pad, 12.0F + pad + static_cast<float>(i) * lineHeight, scale,
                 lines[i], color);
    }

    auto &g = gl();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g.UseProgram(program_);
    const GLint locScreen = g.GetUniformLocation(program_, "u_screenSize");
    if (locScreen >= 0) {
      g.Uniform2f(locScreen, static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    }
    const GLint locTex = g.GetUniformLocation(program_, "u_fontTex");
    if (locTex >= 0) {
      g.Uniform1i(locTex, 0);
    }

    g.ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex_);

    g.BindVertexArray(vao_);
    g.BindBuffer(GL_ARRAY_BUFFER, vbo_);
    g.BufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(QuadVertex)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

    g.BindVertexArray(0);
    g.UseProgram(0);
    glDisable(GL_BLEND);
  }

} // namespace ailivingmaze::render
