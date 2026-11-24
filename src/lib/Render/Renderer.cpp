#include "Render/Renderer.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace ailivingmaze::render {

  Renderer::~Renderer() { shutdown(); }

  void Renderer::destroyMesh(GpuChunkMesh &mesh) {
    auto &g = gl();
    if (!g.loaded) {
      return;
    }
    if (mesh.vbo != 0) {
      g.DeleteBuffers(1, &mesh.vbo);
      mesh.vbo = 0;
    }
    if (mesh.vao != 0) {
      g.DeleteVertexArrays(1, &mesh.vao);
      mesh.vao = 0;
    }
    mesh.vertexCount = 0;
  }

  void Renderer::shutdown() {
    for (auto &[coord, mesh] : gpuChunks_) {
      destroyMesh(mesh);
    }
    gpuChunks_.clear();
  }

  void Renderer::uploadChunk(const world::Chunk &chunk) {
    auto &g = gl();
    GpuChunkMesh mesh{};
    g.GenVertexArrays(1, &mesh.vao);
    g.GenBuffers(1, &mesh.vbo);

    g.BindVertexArray(mesh.vao);
    g.BindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    g.BufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(chunk.vertices.size() * sizeof(world::MeshVertex)),
                 chunk.vertices.data(), GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(sizeof(world::MeshVertex));
    g.EnableVertexAttribArray(0);
    g.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(world::MeshVertex, position)));
    g.EnableVertexAttribArray(1);
    g.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(world::MeshVertex, normal)));
    g.EnableVertexAttribArray(2);
    g.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(world::MeshVertex, uv)));
    g.EnableVertexAttribArray(3);
    g.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(world::MeshVertex, localPos)));
    g.EnableVertexAttribArray(4);
    g.VertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(world::MeshVertex, surfaceType)));

    g.BindVertexArray(0);
    mesh.vertexCount = static_cast<GLsizei>(chunk.vertices.size());
    mesh.version = chunk.meshVersion;

    auto it = gpuChunks_.find(chunk.coord);
    if (it != gpuChunks_.end()) {
      destroyMesh(it->second);
      it->second = mesh;
    } else {
      gpuChunks_.emplace(chunk.coord, mesh);
    }
  }

  void Renderer::syncChunks(const world::ChunkManager &chunkManager) {
    const auto &loaded = chunkManager.loadedChunks();

    // Remove GPU meshes for unloaded chunks
    for (auto it = gpuChunks_.begin(); it != gpuChunks_.end();) {
      if (loaded.find(it->first) == loaded.end()) {
        destroyMesh(it->second);
        it = gpuChunks_.erase(it);
      } else {
        ++it;
      }
    }

    // Upload newly loaded chunks
    for (const auto &[coord, chunk] : loaded) {
      auto it = gpuChunks_.find(coord);
      if (it == gpuChunks_.end() || it->second.version != chunk.meshVersion) {
        uploadChunk(chunk);
      }
    }
  }

  RenderStats Renderer::render(const world::World &world, const Camera &camera,
                               const shader::MaterialManager &materials, float aspectRatio,
                               float elapsedSeconds) {
    auto &g = gl();
    RenderStats stats{};

    const auto playerSector = world::SectorManager::sectorForWorldPos(camera.position());
    const auto playerTheme = world.sectorManager().describeSector(playerSector);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(playerTheme.fogColor.r, playerTheme.fogColor.g, playerTheme.fogColor.b, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 vp = camera.projectionMatrix(aspectRatio) * camera.viewMatrix();
    const Frustum frustum = camera.extractFrustum(vp);

    const auto &chunks = world.chunkManager().loadedChunks();
    stats.loadedChunks = chunks.size();

    GLuint boundProgram = 0;

    for (const auto &[coord, chunk] : chunks) {
      if (!frustum.intersectsAABB(chunk.minBounds, chunk.maxBounds)) {
        ++stats.culledChunks;
        continue;
      }

      const auto gpuIt = gpuChunks_.find(coord);
      if (gpuIt == gpuChunks_.end() || gpuIt->second.vertexCount == 0) {
        continue;
      }

      const GLuint program = materials.resolveProgramForSector(chunk.sectorId);
      if (program == 0) {
        continue;
      }

      if (program != boundProgram) {
        g.UseProgram(program);
        boundProgram = program;
        const GLint locVp = g.GetUniformLocation(program, "u_viewProj");
        if (locVp >= 0) {
          g.UniformMatrix4fv(locVp, 1, GL_FALSE, glm::value_ptr(vp));
        }
        const GLint locTime = g.GetUniformLocation(program, "u_time");
        if (locTime >= 0) {
          g.Uniform1f(locTime, elapsedSeconds);
        }
        const GLint locCam = g.GetUniformLocation(program, "u_cameraPos");
        if (locCam >= 0) {
          g.Uniform3f(locCam, camera.position().x, camera.position().y, camera.position().z);
        }
        const GLint locLightDir = g.GetUniformLocation(program, "u_lightDir");
        if (locLightDir >= 0) {
          g.Uniform3f(locLightDir, 0.35F, 0.88F, 0.32F);
        }
      }

      const auto theme = world.sectorManager().describeSector(chunk.sectorCoord);
      const GLint locSec = g.GetUniformLocation(program, "u_sectorCoord");
      if (locSec >= 0) {
        g.Uniform2f(locSec, static_cast<float>(chunk.sectorCoord.x),
                    static_cast<float>(chunk.sectorCoord.z));
      }
      const GLint locHash = g.GetUniformLocation(program, "u_sectorHash");
      if (locHash >= 0) {
        g.Uniform1i(locHash, chunk.sectorCoord.x * 73856093 ^ chunk.sectorCoord.z * 19349663);
      }
      const GLint locPrim = g.GetUniformLocation(program, "u_primaryColor");
      if (locPrim >= 0) {
        g.Uniform3f(locPrim, theme.primaryColor.r, theme.primaryColor.g, theme.primaryColor.b);
      }
      const GLint locSecCol = g.GetUniformLocation(program, "u_secondaryColor");
      if (locSecCol >= 0) {
        g.Uniform3f(locSecCol, theme.secondaryColor.r, theme.secondaryColor.g,
                    theme.secondaryColor.b);
      }
      const GLint locAcc = g.GetUniformLocation(program, "u_accentColor");
      if (locAcc >= 0) {
        g.Uniform3f(locAcc, theme.accentColor.r, theme.accentColor.g, theme.accentColor.b);
      }
      const GLint locFogCol = g.GetUniformLocation(program, "u_fogColor");
      if (locFogCol >= 0) {
        g.Uniform3f(locFogCol, playerTheme.fogColor.r, playerTheme.fogColor.g,
                    playerTheme.fogColor.b);
      }
      const GLint locFogDens = g.GetUniformLocation(program, "u_fogDensity");
      if (locFogDens >= 0) {
        g.Uniform1f(locFogDens, theme.fogDensity);
      }
      const GLint locLightInt = g.GetUniformLocation(program, "u_lightIntensity");
      if (locLightInt >= 0) {
        g.Uniform1f(locLightInt, theme.lightIntensity);
      }

      g.BindVertexArray(gpuIt->second.vao);
      glDrawArrays(GL_TRIANGLES, 0, gpuIt->second.vertexCount);

      ++stats.renderedChunks;
      ++stats.drawCalls;
      stats.renderedTriangles += static_cast<std::size_t>(gpuIt->second.vertexCount / 3);
    }

    g.BindVertexArray(0);
    g.UseProgram(0);
    return stats;
  }

} // namespace ailivingmaze::render
