// capture.cpp - plays the game with a script and saves screenshots for the README.
//
//   cmake --build build --target capture
//   ./build/capture docs/screenshots
//
// It runs the real game code with scripted input instead of a keyboard, so it
// doubles as a tiny end-to-end test: if the quest can't be finished, it exits
// with an error.
#include <cstdio>
#include <string>

#include "game.hpp"
#include "raylib.h"

using namespace cozy;

static void tick(Game& g, int frames, Input in = {}) {
    for (int i = 0; i < frames; i++) {
        g.update(in, 1.0f / 60.0f);
        in.action = false;  // an action is a single key press, not held
    }
}

static void press(Game& g) {
    Input in;
    in.action = true;
    tick(g, 1, in);
}

static void walk(Game& g, float mx, float my, int frames) {
    Input in;
    in.moveX = mx;
    in.moveY = my;
    tick(g, frames, in);
}

static void shot(Game& g, RenderTexture2D target, const std::string& path) {
    BeginTextureMode(target);
    g.draw();
    EndTextureMode();
    Image img = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&img);
    ImageResizeNN(&img, VIEW_W * 3, VIEW_H * 3);
    ExportImage(img, path.c_str());
    UnloadImage(img);
    std::printf("saved %s\n", path.c_str());
}

static int fail(const char* msg) {
    std::fprintf(stderr, "FAILED: %s\n", msg);
    return 1;
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : ".";

    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(VIEW_W, VIEW_H, "capture");
    RenderTexture2D target = LoadRenderTexture(VIEW_W, VIEW_H);

    Game g;
    g.init();

    // 1. Title screen
    tick(g, 30);
    shot(g, target, dir + "/01-title.png");

    // 2. Start playing, walk up to Mira
    press(g);
    if (g.scene() != Scene::Play) return fail("pressing action on the title should start the game");
    tick(g, 60 * 9);  // let the controls hint fade out
    g.debugPlace(10, 14, Dir::Up);  // just below Mira's fence, facing her
    tick(g, 20);
    shot(g, target, dir + "/02-village.png");

    // 3. Talk to Mira and take the quest
    press(g);
    if (g.scene() != Scene::Talk) return fail("could not start talking to Mira");
    tick(g, 60);
    shot(g, target, dir + "/03-dialogue.png");
    for (int i = 0; i < 12 && g.scene() == Scene::Talk; i++) {
        tick(g, 90);  // let the text finish typing
        press(g);
    }
    if (g.quest() != Quest::Active) return fail("quest should be active after talking to Mira");

    // 4. A mushroom in the woods, then collect all three
    g.debugPlace(kMushroomPos[0].x + 3, kMushroomPos[0].y, Dir::Left);
    tick(g, 20);
    shot(g, target, dir + "/04-mushroom.png");
    for (const TilePos& m : kMushroomPos) {
        g.debugPlace(m.x, m.y, Dir::Down);
        tick(g, 5);
    }
    if (g.quest() != Quest::Ready) return fail("quest should be ready after 3 mushrooms");

    // 5. Hand them in and watch the sky turn golden
    g.debugPlace(10, 14, Dir::Up);
    press(g);
    for (int i = 0; i < 12 && g.scene() == Scene::Talk; i++) {
        tick(g, 90);
        press(g);
    }
    if (g.quest() != Quest::Done) return fail("quest should be done after handing in mushrooms");
    tick(g, 60 * 4);
    shot(g, target, dir + "/05-dusk.png");

    // 6. The pond
    g.debugPlace(31, 18, Dir::Down);
    tick(g, 40);
    shot(g, target, dir + "/06-pond.png");

    // 7. Walking into the forest wall must not let you through it
    g.debugPlace(19, 6, Dir::Up);
    walk(g, 0, -1, 300);
    if (g.playerPos().y < 4 * TILE - 7) return fail("player walked through the trees");

    UnloadRenderTexture(target);
    g.shutdown();
    CloseWindow();
    std::puts("all scripted checks passed");
    return 0;
}
