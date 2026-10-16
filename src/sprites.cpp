// sprites.cpp - pixel art as text.
//
// Each hand-drawn sprite below is 16 rows of 16 characters. A '.' is see-through
// and every other letter is looked up in a palette (see basePalette()).
// To redraw something, just edit the letters. To recolour, change the palette.
#include "sprites.hpp"

#include <cstring>
#include <initializer_list>

namespace cozy {
namespace {

// ---------------------------------------------------------------- palette ---

Color hex(unsigned v) {
    return Color{static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 8),
                 static_cast<unsigned char>(v), 255};
}

struct Palette {
    Color c[128];
    Palette() {
        for (auto& x : c) x = Color{0, 0, 0, 0};
    }
    void set(char k, unsigned rgb) { c[static_cast<int>(k) & 127] = hex(rgb); }
    Color get(char k) const { return c[static_cast<int>(k) & 127]; }
};

Palette basePalette() {
    Palette p;
    p.set('k', 0x2d1b2e);  // outline
    p.set('g', 0x7bc96f);  // grass
    p.set('G', 0x4e9a54);  // dark grass
    p.set('h', 0xa6e08f);  // light grass / leaf highlight
    p.set('t', 0x2c6b4a);  // darkest leaf
    p.set('T', 0x3f9560);  // leaf
    p.set('b', 0x8a5a3c);  // wood
    p.set('B', 0x5e3b2a);  // dark wood
    p.set('s', 0xf2c9a0);  // skin
    p.set('S', 0xd9a47a);  // skin shadow
    p.set('e', 0x2d1b2e);  // eyes
    p.set('r', 0xd95763);  // red
    p.set('R', 0xa63a50);  // dark red
    p.set('w', 0xf6f0e4);  // white
    p.set('m', 0xefe2c4);  // cream
    p.set('X', 0xd9c9a3);  // dark cream
    p.set('y', 0xf3d55b);  // yellow
    p.set('o', 0xe8944a);  // orange (cat)
    p.set('O', 0xc9742f);  // dark orange
    p.set('p', 0xe07a8a);  // pink
    p.set('c', 0x8fd3ee);  // light water / glass
    p.set('a', 0x4f9fd0);  // water
    p.set('d', 0xe0c98f);  // path
    p.set('D', 0xbda46a);  // dark path
    // Character parts: H = hair, C = shirt, U = trousers
    p.set('H', 0x5a3825);
    p.set('C', 0x3fa7a0);
    p.set('U', 0x3b4a7a);
    return p;
}

Palette actorPalette(Look look) {
    Palette p = basePalette();
    switch (look) {
        case Look::Player: break;  // brown hair, teal top, navy trousers
        case Look::Mira:
            p.set('H', 0x8a4fb0);  // purple hair
            p.set('C', 0xf0a1c0);  // pink dress
            p.set('U', 0xc76aa0);
            break;
        case Look::Pip:
            p.set('H', 0xd8d8e0);  // grey hair
            p.set('C', 0x5b8fd9);  // blue coat
            p.set('U', 0x6e5a43);
            break;
        default: break;
    }
    return p;
}

// ------------------------------------------------------------- hand-drawn ---

const char* const kTree[16] = {
    "................",
    "....kkkkkkkk....",
    "...kThhhhTTTk...",
    "..kThhhTTTTTTk..",
    "..kThhTTTTTTTk..",
    ".kTThTTTTTTTTTk.",
    ".kTTTTTTTTTTTTk.",
    ".kTTTTTtTTTTTTk.",
    ".kTTTTTTTTtTTTk.",
    "..kTTTTTTTTTTk..",
    "..kttTTTTTTttk..",
    "...kkttttttkk...",
    ".....kbbBBk.....",
    ".....kbbBBk.....",
    "....kkbbBBkk....",
    "................",
};

const char* const kBush[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "....kkkkkkkk....",
    "...kThhhhTTTk...",
    "..kThhTTTTTTTk..",
    "..kTTTTTTtTTTk..",
    "..kTTtTTTTTTTk..",
    "..kkTTTTTTTTkk..",
    "...kkkkkkkkkk...",
    "................",
    "................",
    "................",
    "................",
};

const char* const kMushroom[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    ".....kkkkkk.....",
    "....krrwrrrk....",
    "...krrrrrwrrk...",
    "...krwrrrrrrk...",
    "...kkkkkkkkkk...",
    ".....kmmmmk.....",
    ".....kmmmmk.....",
    ".....kmXXmk.....",
    "......kkkk......",
    "................",
    "................",
};

// Characters. 'H' hair, 'C' shirt, 'U' trousers, 's' skin, 'e' eyes.
const char* const kFront[16] = {
    "................",
    "....kkkkkkkk....",
    "...kHHHHHHHHk...",
    "...kHHHHHHHHk...",
    "...kHHsssssHk...",
    "...ksessssesk...",
    "....kssssssk....",
    ".....kkkkkk.....",
    "....kCCCCCCk....",
    "...kCCCCCCCCk...",
    "...ksCCCCCCsk...",
    "....kCCCCCCk....",
    "....kUUUUUUk....",
    "....kUUkkUUk....",
    "....kkk..kkk....",
    "................",
};

const char* const kBack[16] = {
    "................",
    "....kkkkkkkk....",
    "...kHHHHHHHHk...",
    "...kHHHHHHHHk...",
    "...kHHHHHHHHk...",
    "...kHHHHHHHHk...",
    "....kHHHHHHk....",
    ".....kkkkkk.....",
    "....kCCCCCCk....",
    "...kCCCCCCCCk...",
    "...ksCCCCCCsk...",
    "....kCCCCCCk....",
    "....kUUUUUUk....",
    "....kUUkkUUk....",
    "....kkk..kkk....",
    "................",
};

// Faces right. The left-facing version is just this drawn flipped.
const char* const kSide[16] = {
    "................",
    "....kkkkkkk.....",
    "...kHHHHHHHk....",
    "...kHHHHHHHHk...",
    "...kHHHssssk....",
    "...kHHHsssesk...",
    "....kHHssssk....",
    ".....kkkkkk.....",
    ".....kCCCCk.....",
    ".....kCCCCCk....",
    ".....kCCsCCk....",
    ".....kCCCCk.....",
    ".....kUUUUk.....",
    ".....kUUUUk.....",
    ".....kkkkkk.....",
    "................",
};

// Same as kSide but the legs are apart (mid-step).
const char* const kSideStep[16] = {
    "................",
    "....kkkkkkk.....",
    "...kHHHHHHHk....",
    "...kHHHHHHHHk...",
    "...kHHHssssk....",
    "...kHHHsssesk...",
    "....kHHssssk....",
    ".....kkkkkk.....",
    ".....kCCCCk.....",
    ".....kCCCCCk....",
    ".....kCCsCCk....",
    ".....kCCCCk.....",
    ".....kUUUUk.....",
    ".....kUkkUk.....",
    "....kkk..kkk....",
    "................",
};

// A little cat. 'e' = eyes (replaced when blinking).
const char* const kCat[16] = {
    "................",
    "................",
    "................",
    "................",
    "....kk....kk....",
    "....kok..kok....",
    "....kooooook....",
    "....koeooeok....",
    "....kooppook....",
    ".....kwwwwk.....",
    "....kooooook....",
    "...koowwwwook...",
    "...koowwwwookk..",
    "...kooooooookok.",
    "...kwwkkkkwwkk..",
    "................",
};

// ----------------------------------------------------------------- canvas ---

// A 16x16 window into a bigger image, so tiles can be painted straight into
// the tile atlas.
struct Canvas {
    Image* img;
    int ox;
    void px(int x, int y, Color c) const {
        if (x < 0 || y < 0 || x >= TILE || y >= TILE) return;
        ImageDrawPixel(img, ox + x, y, c);
    }
    void rect(int x, int y, int w, int h, Color c) const {
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++) px(x + i, y + j, c);
    }
    void fill(Color c) const { rect(0, 0, TILE, TILE, c); }
    void frame(int x, int y, int w, int h, Color c) const {
        rect(x, y, w, 1, c);
        rect(x, y + h - 1, w, 1, c);
        rect(x, y, 1, h, c);
        rect(x + w - 1, y, 1, h, c);
    }
};

void blitRows(const Canvas& cv, const char* const rows[16], const Palette& pal,
              bool blink = false) {
    for (int y = 0; y < TILE; y++) {
        const size_t n = std::strlen(rows[y]);
        if (n != TILE) TraceLog(LOG_ERROR, "sprite row %d has %d chars (expected 16)", y, (int)n);
        for (size_t x = 0; x < n && x < TILE; x++) {
            char ch = rows[y][x];
            if (blink && ch == 'e') ch = 'o';  // closed eyes: just skip the dark pixels
            cv.px(static_cast<int>(x), y, pal.get(ch));
        }
    }
}

// ------------------------------------------------------ generated tiles ---

void genGrass(const Canvas& cv, const Palette& p, int variant) {
    cv.fill(p.get('g'));
    for (int i = 0; i < 9; i++) {
        unsigned h = hash2(variant * 17 + i, 5, 11);
        int x = static_cast<int>(h % 16), y = static_cast<int>((h >> 8) % 16);
        Color c = (i % 2) ? p.get('G') : p.get('h');
        cv.px(x, y, c);
        if (i % 3 == 0) cv.px(x, y - 1, c);  // little tuft
    }
}

void genPath(const Canvas& cv, const Palette& p) {
    cv.fill(p.get('d'));
    for (int i = 0; i < 10; i++) {
        unsigned h = hash2(i, 3, 29);
        int x = static_cast<int>(h % 15), y = static_cast<int>((h >> 8) % 16);
        cv.px(x, y, p.get('D'));
        if (i % 2 == 0) cv.px(x + 1, y, p.get('D'));
    }
}

void genWater(const Canvas& cv, const Palette& p, int frame) {
    cv.fill(p.get('a'));
    const int shift = frame * 4;
    for (int row = 0; row < 4; row++) {
        const int y = 2 + row * 4;
        const int x = (row % 2 ? 6 : 1) + shift;
        for (int i = 0; i < 4; i++) cv.px((x + i) % TILE, y, p.get('c'));
    }
}

void genRoof(const Canvas& cv, const Palette& p, bool left, bool right, bool eave) {
    for (int y = 0; y < TILE; y++) {
        for (int x = 0; x < TILE; x++) {
            Color c = p.get('r');
            if (y % 4 == 3) c = p.get('R');
            else if ((x + ((y / 4) % 2) * 4) % 8 == 0) c = p.get('R');
            cv.px(x, y, c);
        }
    }
    if (!eave) cv.rect(0, 0, TILE, 1, p.get('k'));  // ridge line
    if (eave) {
        cv.rect(0, 13, TILE, 2, p.get('R'));
        cv.rect(0, 15, TILE, 1, p.get('k'));
    }
    if (left) cv.rect(0, 0, 1, TILE, p.get('k'));
    if (right) cv.rect(TILE - 1, 0, 1, TILE, p.get('k'));
}

void genWall(const Canvas& cv, const Palette& p, bool left, bool right) {
    cv.fill(p.get('m'));
    for (int y = 4; y < TILE; y += 5) cv.rect(0, y, TILE, 1, p.get('X'));
    cv.rect(0, 15, TILE, 1, p.get('k'));
    if (left) cv.rect(0, 0, 1, TILE, p.get('k'));
    if (right) cv.rect(TILE - 1, 0, 1, TILE, p.get('k'));
}

void genWindow(const Canvas& cv, const Palette& p) {
    genWall(cv, p, false, false);
    cv.rect(3, 3, 10, 8, p.get('c'));
    cv.frame(3, 3, 10, 8, p.get('B'));
    cv.rect(7, 3, 2, 8, p.get('B'));
    cv.rect(3, 6, 10, 2, p.get('B'));
    cv.px(4, 4, p.get('w'));
    cv.rect(2, 11, 12, 2, p.get('b'));  // sill
}

void genDoor(const Canvas& cv, const Palette& p) {
    genWall(cv, p, false, false);
    cv.rect(4, 2, 8, 14, p.get('b'));
    cv.frame(4, 2, 8, 14, p.get('B'));
    cv.rect(7, 3, 2, 12, p.get('B'));  // a seam down the middle
    cv.px(10, 9, p.get('y'));          // door knob
    cv.rect(3, 15, 10, 1, p.get('k'));
}

void genFence(const Canvas& cv, const Palette& p) {
    cv.rect(0, 7, TILE, 2, p.get('b'));
    cv.rect(0, 11, TILE, 2, p.get('b'));
    cv.rect(0, 8, TILE, 1, p.get('B'));
    cv.rect(0, 12, TILE, 1, p.get('B'));
    for (int x : {2, 12}) {
        cv.rect(x, 5, 2, 10, p.get('b'));
        cv.rect(x - 1, 5, 1, 10, p.get('k'));
        cv.rect(x + 2, 5, 1, 10, p.get('k'));
        cv.rect(x, 4, 2, 1, p.get('k'));
        cv.rect(x + 1, 6, 1, 8, p.get('B'));
    }
}

void genSign(const Canvas& cv, const Palette& p) {
    cv.rect(7, 8, 2, 7, p.get('B'));
    cv.rect(2, 2, 12, 7, p.get('d'));
    cv.frame(2, 2, 12, 7, p.get('k'));
    cv.rect(4, 4, 8, 1, p.get('D'));
    cv.rect(4, 6, 6, 1, p.get('D'));
}

void genFlower(const Canvas& cv, const Palette& p, bool red) {
    const Color petal = red ? p.get('r') : p.get('y');
    const Color mid = red ? p.get('w') : p.get('o');
    struct Pos { int x, y; };
    for (Pos f : {Pos{4, 5}, Pos{11, 10}}) {
        cv.px(f.x, f.y - 1, petal);
        cv.px(f.x, f.y + 1, petal);
        cv.px(f.x - 1, f.y, petal);
        cv.px(f.x + 1, f.y, petal);
        cv.px(f.x, f.y, mid);
        cv.px(f.x, f.y + 2, p.get('G'));
        cv.px(f.x, f.y + 3, p.get('G'));
    }
}

}  // namespace

// ------------------------------------------------------------------ atlas ---

void Atlas::load() {
    // The default font is blocky pixel text, so keep it crisp when scaled.
    SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_POINT);

    const Palette pal = basePalette();
    const int n = static_cast<int>(Tile::Count);
    Image atlas = GenImageColor(TILE * n, TILE, BLANK);

    for (int i = 0; i < n; i++) {
        Canvas cv{&atlas, i * TILE};
        switch (static_cast<Tile>(i)) {
            case Tile::GrassA: genGrass(cv, pal, 0); break;
            case Tile::GrassB: genGrass(cv, pal, 1); break;
            case Tile::Path: genPath(cv, pal); break;
            case Tile::WaterA: genWater(cv, pal, 0); break;
            case Tile::WaterB: genWater(cv, pal, 1); break;
            case Tile::Tree: blitRows(cv, kTree, pal); break;
            case Tile::Bush: blitRows(cv, kBush, pal); break;
            case Tile::FlowerRed: genFlower(cv, pal, true); break;
            case Tile::FlowerYellow: genFlower(cv, pal, false); break;
            case Tile::Fence: genFence(cv, pal); break;
            case Tile::Sign: genSign(cv, pal); break;
            case Tile::RoofL: genRoof(cv, pal, true, false, false); break;
            case Tile::RoofM: genRoof(cv, pal, false, false, false); break;
            case Tile::RoofR: genRoof(cv, pal, false, true, false); break;
            case Tile::EaveL: genRoof(cv, pal, true, false, true); break;
            case Tile::EaveM: genRoof(cv, pal, false, false, true); break;
            case Tile::EaveR: genRoof(cv, pal, false, true, true); break;
            case Tile::WallL: genWall(cv, pal, true, false); break;
            case Tile::WallR: genWall(cv, pal, false, true); break;
            case Tile::Window: genWindow(cv, pal); break;
            case Tile::Door: genDoor(cv, pal); break;
            default: break;
        }
    }
    tiles_ = LoadTextureFromImage(atlas);
    SetTextureFilter(tiles_, TEXTURE_FILTER_POINT);
    UnloadImage(atlas);

    auto make = [](const char* const rows[16], const Palette& p, bool blink = false) {
        Image img = GenImageColor(TILE, TILE, BLANK);
        blitRows(Canvas{&img, 0}, rows, p, blink);
        Texture2D t = LoadTextureFromImage(img);
        SetTextureFilter(t, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        return t;
    };

    for (int l = 0; l < static_cast<int>(Look::Count); l++) {
        const Palette ap = actorPalette(static_cast<Look>(l));
        actors_[l][0] = make(kFront, ap);
        actors_[l][1] = make(kBack, ap);
        actors_[l][2] = make(kSide, ap);
        actors_[l][3] = make(kSideStep, ap);
    }
    cat_[0] = make(kCat, pal);
    cat_[1] = make(kCat, pal, true);
    mushroom_ = make(kMushroom, pal);
}

void Atlas::unload() {
    UnloadTexture(tiles_);
    for (auto& look : actors_)
        for (auto& t : look) UnloadTexture(t);
    UnloadTexture(cat_[0]);
    UnloadTexture(cat_[1]);
    UnloadTexture(mushroom_);
}

void Atlas::drawTile(Tile t, int px, int py, unsigned variant) const {
    const auto draw = [&](Tile which) {
        Rectangle src{static_cast<float>(static_cast<int>(which) * TILE), 0, TILE, TILE};
        DrawTextureRec(tiles_, src, Vector2{static_cast<float>(px), static_cast<float>(py)}, WHITE);
    };
    if (isOverlay(t)) draw((variant & 1) ? Tile::GrassB : Tile::GrassA);
    draw(t);
}

void Atlas::drawActor(Look look, Dir dir, int frame, float x, float y) const {
    const auto& set = actors_[static_cast<int>(look)];
    const Texture2D* tex = &set[0];
    bool flip = false;
    float bob = 0;
    switch (dir) {
        case Dir::Down: tex = &set[0]; flip = frame == 1; break;
        case Dir::Up: tex = &set[1]; flip = frame == 1; break;
        case Dir::Left: flip = true; tex = &set[frame == 1 ? 3 : 2]; break;
        case Dir::Right: tex = &set[frame == 1 ? 3 : 2]; break;
    }
    if (frame == 1) bob = -1;  // a tiny hop makes the walk feel alive
    Rectangle src{0, 0, flip ? -16.0f : 16.0f, 16};
    DrawTextureRec(*tex, src, Vector2{x, y + bob}, WHITE);
}

void Atlas::drawCat(bool blink, float x, float y) const {
    DrawTextureV(cat_[blink ? 1 : 0], Vector2{x, y}, WHITE);
}

void Atlas::drawMushroom(float x, float y) const {
    DrawTextureV(mushroom_, Vector2{x, y}, WHITE);
}

void Atlas::drawMushroomIcon(int x, int y, int size) const {
    DrawTexturePro(mushroom_, Rectangle{2, 4, 12, 12},
                   Rectangle{static_cast<float>(x), static_cast<float>(y),
                             static_cast<float>(size), static_cast<float>(size)},
                   Vector2{0, 0}, 0, WHITE);
}

}  // namespace cozy
