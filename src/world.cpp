#include "world.hpp"

#include <algorithm>

namespace cozy {

World::World() { generate(); }

Tile World::at(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= W || ty >= H) return Tile::Tree;  // the world is ringed by forest
    return tiles_[ty * W + tx];
}

bool World::solid(int tx, int ty) const { return isSolid(at(tx, ty)); }

bool World::isSolid(Tile t) {
    switch (t) {
        case Tile::GrassA:
        case Tile::GrassB:
        case Tile::Path:
        case Tile::FlowerRed:
        case Tile::FlowerYellow:
            return false;
        default:
            return true;
    }
}

void World::set(int x, int y, Tile t) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    tiles_[y * W + x] = t;
}

void World::generate() {
    tiles_.fill(Tile::GrassA);

    auto path = [&](int x0, int y0, int x1, int y1) {
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) set(x, y, Tile::Path);
    };

    // A house is 4 tiles wide and 3 tall: two rows of roof, then a wall row.
    auto house = [&](int x, int y, int doorIdx) {
        set(x, y, Tile::RoofL);
        set(x + 1, y, Tile::RoofM);
        set(x + 2, y, Tile::RoofM);
        set(x + 3, y, Tile::RoofR);
        set(x, y + 1, Tile::EaveL);
        set(x + 1, y + 1, Tile::EaveM);
        set(x + 2, y + 1, Tile::EaveM);
        set(x + 3, y + 1, Tile::EaveR);
        for (int i = 0; i < 4; i++) {
            Tile t = Tile::Window;
            if (i == 0) t = Tile::WallL;
            else if (i == 3) t = Tile::WallR;
            else if (i == doorIdx) t = Tile::Door;
            set(x + i, y + 2, t);
        }
    };

    // Main roads: one east-west, one north-south.
    path(2, 14, 37, 15);
    path(19, 4, 20, 27);

    // Houses and the little paths from their doors.
    house(6, 8, 2);          // Mira's house, door at x=8
    path(8, 11, 8, 13);
    house(26, 8, 2);         // house B, door at x=28
    path(28, 11, 28, 13);
    house(6, 19, 2);         // house C, door at x=8
    path(8, 22, 19, 22);

    // Garden fence in front of Mira's house (with a gap for the path).
    for (int x = 5; x <= 11; x++)
        if (x != 8) set(x, 12, Tile::Fence);

    // The pond.
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const float dx = (x - 32.5f) / 4.5f, dy = (y - 23.0f) / 3.2f;
            if (dx * dx + dy * dy < 1.0f) set(x, y, Tile::WaterA);
        }

    set(kSignPos.x, kSignPos.y, Tile::Sign);

    // Forest all the way around the edge (but never on top of a road).
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const bool border = y <= 3 || y >= 28 || x <= 1 || x >= 38;
            if (border && at(x, y) == Tile::GrassA) set(x, y, Tile::Tree);
        }

    // Work out where we must NOT scatter random trees: around paths, houses,
    // water and every important spot (so you can always walk where you need to).
    std::array<bool, W * H> reserved{};
    auto reserve = [&](int cx, int cy, int r) {
        for (int y = cy - r; y <= cy + r; y++)
            for (int x = cx - r; x <= cx + r; x++)
                if (x >= 0 && y >= 0 && x < W && y < H) reserved[y * W + x] = true;
    };
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const Tile t = at(x, y);
            if (t != Tile::GrassA && t != Tile::Tree) reserve(x, y, 1);
        }
    reserve(kPlayerStart.x, kPlayerStart.y, 2);
    reserve(kMiraPos.x, kMiraPos.y, 2);
    reserve(kPipPos.x, kPipPos.y, 2);
    reserve(kCatPos.x, kCatPos.y, 2);
    for (const TilePos& m : kMushroomPos) reserve(m.x, m.y, 1);

    // Scatter trees, bushes and flowers. hash2 makes this deterministic.
    for (int y = 4; y < 28; y++)
        for (int x = 2; x < 38; x++) {
            if (at(x, y) != Tile::GrassA) continue;
            const unsigned h = hash2(x, y, 7) % 100;
            if (!reserved[y * W + x]) {
                const unsigned treeChance = (y <= 5) ? 45 : 7;  // thicker woods in the north
                if (h < treeChance) { set(x, y, Tile::Tree); continue; }
                if (h < treeChance + 3) { set(x, y, Tile::Bush); continue; }
            }
            if (hash2(x, y, 13) % 100 < 12)
                set(x, y, (hash2(x, y, 9) & 1) ? Tile::FlowerRed : Tile::FlowerYellow);
        }

    // Make sure the mushroom spots themselves are open grass.
    for (const TilePos& m : kMushroomPos) set(m.x, m.y, Tile::GrassA);
}

}  // namespace cozy
