// game.hpp - the game itself: state, input handling, update and drawing.
//
// Game knows nothing about windows or keyboards. It is fed an Input every
// frame and draws into whatever render target is currently active. That keeps
// main.cpp tiny and lets tools/capture.cpp drive the game without a keyboard.
#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "sprites.hpp"
#include "world.hpp"

namespace cozy {

constexpr int VIEW_W = 320;  // the game is drawn at this tiny size...
constexpr int VIEW_H = 240;  // ...then scaled up, which is what keeps the pixels crisp

struct Input {
    float moveX = 0;  // -1 left ... +1 right
    float moveY = 0;  // -1 up ... +1 down
    bool action = false;  // "talk / confirm" was pressed this frame
};

enum class Scene { Title, Play, Talk };
enum class Quest { NotStarted, Active, Ready, Done };

class Game {
public:
    void init();  // after InitWindow()
    void shutdown();

    void update(const Input& in, float dt);
    void draw() const;  // draws a VIEW_W x VIEW_H frame

    Scene scene() const { return scene_; }
    Quest quest() const { return quest_; }
    Vector2 playerPos() const { return player_; }

    // Used by tools/capture.cpp (screenshots). Not needed for normal play.
    void debugPlace(int tileX, int tileY, Dir facing);

private:
    struct Npc {
        Look look;
        Dir dir;
        Vector2 pos;
        const char* name;
    };
    struct Mushroom {
        TilePos tile;
        bool taken = false;
    };
    struct Talk {
        std::string speaker;
        std::vector<std::string> pages;
        size_t page = 0;
        std::vector<std::string> lines;  // current page, word-wrapped
        int totalChars = 0;
        float shown = 0;  // how many characters have been typed out so far
        std::function<void()> onEnd;
    };

    // --- update helpers
    void updatePlay(const Input& in, float dt);
    void updateTalk(const Input& in, float dt);
    void movePlayer(const Input& in, float dt);
    bool blockedAt(Vector2 p) const;
    void interact();
    void talkToNpc(size_t index);
    void startTalk(std::string speaker, std::vector<std::string> pages,
                   std::function<void()> onEnd = {});
    void loadPage();
    void showToast(const std::string& text);

    // --- draw helpers
    void drawWorld() const;
    void drawActors() const;
    void drawHud() const;
    void drawTalk() const;
    void drawTitle() const;
    void drawToast() const;

    Atlas atlas_;
    World world_;

    Scene scene_ = Scene::Title;
    Quest quest_ = Quest::NotStarted;

    Vector2 player_{};
    Dir facing_ = Dir::Down;
    bool moving_ = false;
    float walkTime_ = 0;

    std::vector<Npc> npcs_;
    Vector2 catPos_{};
    int catTalks_ = 0;

    std::array<Mushroom, 3> mushrooms_{};
    int mushroomCount_ = 0;

    Talk talk_;

    Vector2 cam_{};
    float time_ = 0;
    float dusk_ = 0;  // 0..1, the sky turns golden once the quest is done
    float toastTime_ = 0;
    std::string toast_;
    float hintTime_ = 0;
};

}  // namespace cozy
