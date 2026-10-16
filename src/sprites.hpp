// sprites.hpp - all the pixel art in the game, plus the code that draws it.
//
// Every sprite is a 16x16 grid of characters (see sprites.cpp). Each character
// maps to a colour in a palette, so you can recolour or redraw anything just by
// editing text. No external image files are needed.
#pragma once

#include "raylib.h"

namespace cozy {

constexpr int TILE = 16;  // size of one tile / sprite in pixels

// A tiny integer hash. Gives the same "random" number for the same inputs, so
// the village looks identical on every run without storing any map data.
inline unsigned hash2(int x, int y, unsigned seed = 0) {
    unsigned h = static_cast<unsigned>(x) * 374761393u +
                 static_cast<unsigned>(y) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

enum class Tile : unsigned char {
    GrassA, GrassB, Path, WaterA, WaterB,
    Tree, Bush, FlowerRed, FlowerYellow, Fence, Sign,
    RoofL, RoofM, RoofR, EaveL, EaveM, EaveR,
    WallL, WallR, Window, Door,
    Count
};

enum class Dir : unsigned char { Down, Up, Left, Right };

// Which character palette to use. Same drawing, different hair / clothes.
enum class Look : unsigned char { Player, Mira, Pip, Count };

// Tiles that are drawn on top of a grass tile (they have see-through pixels).
inline bool isOverlay(Tile t) {
    return t == Tile::Tree || t == Tile::Bush || t == Tile::FlowerRed ||
           t == Tile::FlowerYellow || t == Tile::Fence || t == Tile::Sign;
}

class Atlas {
public:
    void load();    // call once, after InitWindow()
    void unload();

    void drawTile(Tile t, int px, int py, unsigned variant = 0) const;
    // frame 0 = standing, frame 1 = mid-step (used for the walk animation)
    void drawActor(Look look, Dir dir, int frame, float x, float y) const;
    void drawCat(bool blink, float x, float y) const;
    void drawMushroom(float x, float y) const;
    void drawMushroomIcon(int x, int y, int size) const;

private:
    Texture2D tiles_{};
    Texture2D actors_[static_cast<int>(Look::Count)][4]{};  // down, up, side, side-step
    Texture2D cat_[2]{};
    Texture2D mushroom_{};
};

}  // namespace cozy
