// main.cpp - opens the window and runs the game loop.
#include <algorithm>
#include <cmath>

#include "game.hpp"
#include "raylib.h"

using namespace cozy;

static Input readInput() {
    Input in;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) in.moveX += 1;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) in.moveX -= 1;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) in.moveY += 1;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) in.moveY -= 1;
    in.action = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z) ||
                IsKeyPressed(KEY_E);
    return in;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VIEW_W * 3, VIEW_H * 3, "Willowbrook - a cozy village");
    SetWindowMinSize(VIEW_W, VIEW_H);
    SetTargetFPS(60);

    Game game;
    game.init();

    // The game draws into a small 320x240 texture which we scale up to fit the
    // window. That's the trick that keeps pixel art sharp at any window size.
    RenderTexture2D canvas = LoadRenderTexture(VIEW_W, VIEW_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose()) {
        game.update(readInput(), std::min(GetFrameTime(), 0.05f));

        BeginTextureMode(canvas);
        game.draw();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        const float scale = std::max(1.0f, std::floor(std::min(GetScreenWidth() / static_cast<float>(VIEW_W),
                                                                 GetScreenHeight() / static_cast<float>(VIEW_H))));
        const float w = VIEW_W * scale, h = VIEW_H * scale;
        const Rectangle dest{(GetScreenWidth() - w) / 2, (GetScreenHeight() - h) / 2, w, h};
        // Negative height: render textures are stored upside-down.
        DrawTexturePro(canvas.texture, Rectangle{0, 0, VIEW_W, -VIEW_H}, dest, Vector2{0, 0}, 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    game.shutdown();
    CloseWindow();
    return 0;
}
