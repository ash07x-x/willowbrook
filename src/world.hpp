// world.hpp - the village map.
//
// The map is built in code (see world.cpp) rather than loaded from a file, so
// it is easy to tweak: move a house, add a path, grow the pond.
#pragma once

#include <array>

#include "sprites.hpp"

namespace cozy {

struct TilePos {
    int x, y;
};

// Where things start. Edit these to rearrange the village.
constexpr TilePos kPlayerStart{19, 17};
constexpr TilePos kMiraPos{10, 13};
constexpr TilePos kPipPos{23, 13};
constexpr TilePos kCatPos{14, 17};
constexpr TilePos kSignPos{18, 13};
constexpr std::array<TilePos, 3> kMushroomPos{{{4, 6}, {33, 7}, {12, 25}}};

class World {
public:
    static constexpr int W = 40;  // map size in tiles
    static constexpr int H = 30;

    World();

    Tile at(int tx, int ty) const;
    bool solid(int tx, int ty) const;  // true for anything you can't walk through
    unsigned variant(int tx, int ty) const { return hash2(tx, ty, 3); }

    static bool isSolid(Tile t);

private:
    void generate();
    void set(int x, int y, Tile t);

    std::array<Tile, W * H> tiles_{};
};

}  // namespace cozy
