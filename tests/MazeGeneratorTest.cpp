#include "World/MazeGenerator.hpp"

#include <gtest/gtest.h>

using namespace ailivingmaze::world;

TEST(MazeGeneratorTest, DeterministicGenerationForSameSeedAndCoord) {
  MazeGenerator genA(424242ULL);
  MazeGenerator genB(424242ULL);

  const auto c1 = genA.generateChunk({3, -7});
  const auto c2 = genB.generateChunk({3, -7});

  EXPECT_EQ(c1.coord, c2.coord);
  EXPECT_EQ(c1.cells, c2.cells);
}

TEST(MazeGeneratorTest, DifferentSeedsProduceDifferentChunks) {
  MazeGenerator genA(100ULL);
  MazeGenerator genB(200ULL);

  const auto c1 = genA.generateChunk({0, 0});
  const auto c2 = genB.generateChunk({0, 0});

  EXPECT_NE(c1.cells, c2.cells);
}

TEST(MazeGeneratorTest, EveryGeneratedChunkIsFullyConnected) {
  MazeGenerator gen(1337ULL);
  for (int z = -3; z <= 3; ++z) {
    for (int x = -3; x <= 3; ++x) {
      const auto chunk = gen.generateChunk({x, z});
      EXPECT_TRUE(MazeGenerator::verifyChunkConnectivity(chunk))
          << "Chunk (" << x << ", " << z << ") failed connectivity check";
    }
  }
}

TEST(MazeGeneratorTest, AdjacentChunksShareExactBorderPortals) {
  MazeGenerator gen(98765ULL);
  const auto center = gen.generateChunk({0, 0});
  const auto east = gen.generateChunk({1, 0});
  const auto south = gen.generateChunk({0, 1});

  int sharedEastWest = 0;
  for (int z = 0; z < kChunkCells; ++z) {
    const bool centerOpenEast = (center.cellAt(kChunkCells - 1, z) & WALL_EAST) == 0;
    const bool eastOpenWest = (east.cellAt(0, z) & WALL_WEST) == 0;
    EXPECT_EQ(centerOpenEast, eastOpenWest) << "Mismatch on East/West border at row " << z;
    if (centerOpenEast) {
      ++sharedEastWest;
    }
  }
  EXPECT_GE(sharedEastWest, 1);

  int sharedSouthNorth = 0;
  for (int x = 0; x < kChunkCells; ++x) {
    const bool centerOpenSouth = (center.cellAt(x, kChunkCells - 1) & WALL_SOUTH) == 0;
    const bool southOpenNorth = (south.cellAt(x, 0) & WALL_NORTH) == 0;
    EXPECT_EQ(centerOpenSouth, southOpenNorth) << "Mismatch on South/North border at col " << x;
    if (centerOpenSouth) {
      ++sharedSouthNorth;
    }
  }
  EXPECT_GE(sharedSouthNorth, 1);
}
