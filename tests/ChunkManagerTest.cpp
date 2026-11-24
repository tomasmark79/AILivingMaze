#include "Game/CollisionSystem.hpp"
#include "World/ChunkManager.hpp"
#include "World/World.hpp"

#include <gtest/gtest.h>

using namespace ailivingmaze::world;
using namespace ailivingmaze::game;

TEST(ChunkManagerTest, DynamicallyLoadsAndUnloadsAroundPlayer) {
  MazeGenerator gen(1337ULL);
  ChunkManager mgr(gen, /*loadRadius=*/1, /*maxLoadedChunks=*/25);

  mgr.updateAround({2.0F, 1.7F, 2.0F});
  EXPECT_EQ(mgr.loadedCount(), 9U);
  EXPECT_NE(mgr.findChunk({0, 0}), nullptr);
  EXPECT_NE(mgr.findChunk({1, 1}), nullptr);

  // Move far away so (0,0) must be unloaded and new chunks around (10, 10) loaded
  const glm::vec3 farPos{10.5F * kChunkWorldSize, 1.7F, 10.5F * kChunkWorldSize};
  mgr.updateAround(farPos);

  EXPECT_EQ(mgr.findChunk({0, 0}), nullptr);
  EXPECT_NE(mgr.findChunk({10, 10}), nullptr);
  EXPECT_EQ(mgr.loadedCount(), 9U);
}

TEST(ChunkManagerTest, ChunkMeshesAreGeneratedWithBounds) {
  MazeGenerator gen(777ULL);
  ChunkManager mgr(gen, 0, 4);
  mgr.updateAround({2.0F, 1.7F, 2.0F});

  const Chunk *c = mgr.findChunk({0, 0});
  ASSERT_NE(c, nullptr);
  EXPECT_GT(c->vertices.size(), 100U);
  EXPECT_EQ(c->vertices.size() % 3U, 0U);
  EXPECT_FLOAT_EQ(c->minBounds.x, 0.0F);
  EXPECT_FLOAT_EQ(c->maxBounds.x, kChunkWorldSize);
}

TEST(CollisionSystemTest, BlocksMovementThroughSolidWall) {
  World world(1337ULL, 1);
  world.update(World::defaultSpawnPosition());
  CollisionSystem collision(world);

  const auto spawn = World::defaultSpawnPosition();
  const uint8_t walls = world.cellWallsAtGlobal(0, 0);

  // Find a direction with a solid wall in cell (0,0) and attempt to walk through it
  glm::vec3 target = spawn;
  bool tested = false;
  if ((walls & WALL_NORTH) != 0) {
    target.z -= kCellSize;
    tested = true;
  } else if ((walls & WALL_WEST) != 0) {
    target.x -= kCellSize;
    tested = true;
  } else if ((walls & WALL_SOUTH) != 0) {
    target.z += kCellSize;
    tested = true;
  } else if ((walls & WALL_EAST) != 0) {
    target.x += kCellSize;
    tested = true;
  }

  ASSERT_TRUE(tested);
  const glm::vec3 resolved = collision.resolveMovement(spawn, target, 0.5F);
  // Player must remain inside cell (0,0) bounds [0, kCellSize]
  EXPECT_GT(resolved.x, 0.0F);
  EXPECT_LT(resolved.x, kCellSize);
  EXPECT_GT(resolved.z, 0.0F);
  EXPECT_LT(resolved.z, kCellSize);
}
