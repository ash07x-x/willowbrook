#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace cozy {
namespace {

// --- a small palette for the UI, so the text matches the pixel art
const Color kCream{246, 240, 228, 255};
const Color kInk{45, 27, 46, 255};
const Color kPanel{30, 22, 38, 246};
const Color kGold{243, 213, 91, 255};

constexpr float kWalkSpeed = 58.0f;       // pixels per second
constexpr float kTypeSpeed = 48.0f;       // characters per second in dialogue
constexpr int kTextSize = 10;             // raylib's default font is designed for 10px

Vector2 dirVec(Dir d) {
    switch (d) {
        case Dir::Up: return {0, -1};
        case Dir::Down: return {0, 1};
        case Dir::Left: return {-1, 0};
        case Dir::Right: return {1, 0};
    }
    return {0, 0};
}

Dir opposite(Dir d) {
    switch (d) {
        case Dir::Up: return Dir::Down;
        case Dir::Down: return Dir::Up;
        case Dir::Left: return Dir::Right;
        case Dir::Right: return Dir::Left;
    }
    return Dir::Down;
}

// The part of a character that blocks movement (roughly the lower body).
Rectangle bodyRect(Vector2 p) { return Rectangle{p.x + 3, p.y + 7, 10, 8}; }

Rectangle inflate(Rectangle r, float by) {
    return Rectangle{r.x - by, r.y - by, r.width + by * 2, r.height + by * 2};
}

Vector2 tileToPixel(TilePos t) {
    return Vector2{static_cast<float>(t.x * TILE), static_cast<float>(t.y * TILE)};
}

void drawText(const std::string& s, int x, int y, int size, Color c, bool centered = false) {
    if (centered) x -= MeasureText(s.c_str(), size) / 2;
    DrawText(s.c_str(), x + 1, y + 1, size, kInk);  // drop shadow keeps text readable on any background
    DrawText(s.c_str(), x, y, size, c);
}

std::vector<std::string> wrapText(const std::string& s, int maxWidth) {
    std::vector<std::string> lines;
    std::istringstream words(s);
    std::string word, cur;
    while (words >> word) {
        const std::string attempt = cur.empty() ? word : cur + " " + word;
        if (!cur.empty() && MeasureText(attempt.c_str(), kTextSize) > maxWidth) {
            lines.push_back(cur);
            cur = word;
        } else {
            cur = attempt;
        }
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

}  // namespace

// ------------------------------------------------------------------ setup ---

void Game::init() {
    atlas_.load();

    player_ = tileToPixel(kPlayerStart);
    npcs_ = {
        {Look::Mira, Dir::Down, tileToPixel(kMiraPos), "Mira"},
        {Look::Pip, Dir::Down, tileToPixel(kPipPos), "Pip"},
    };
    catPos_ = tileToPixel(kCatPos);
    for (size_t i = 0; i < mushrooms_.size(); i++) mushrooms_[i] = {kMushroomPos[i], false};
}

void Game::shutdown() { atlas_.unload(); }

void Game::debugPlace(int tileX, int tileY, Dir facing) {
    player_ = tileToPixel({tileX, tileY});
    facing_ = facing;
    if (scene_ == Scene::Title) scene_ = Scene::Play;
}

// ----------------------------------------------------------------- update ---

void Game::update(const Input& in, float dt) {
    time_ += dt;
    if (toastTime_ > 0) toastTime_ -= dt;
    if (quest_ == Quest::Done) dusk_ = std::min(1.0f, dusk_ + dt * 0.25f);

    switch (scene_) {
        case Scene::Title:
            // A slow pan across the village behind the title card.
            cam_.x = std::floor(170 + std::sin(time_ * 0.35f) * 70);
            cam_.y = 96;
            if (in.action) {
                scene_ = Scene::Play;
                hintTime_ = 8.0f;
            }
            return;
        case Scene::Play: updatePlay(in, dt); break;
        case Scene::Talk: updateTalk(in, dt); break;
    }

    // Camera follows the player, but never shows beyond the edge of the map.
    cam_.x = std::floor(std::clamp(player_.x + 8 - VIEW_W / 2.0f, 0.0f,
                                   static_cast<float>(World::W * TILE - VIEW_W)));
    cam_.y = std::floor(std::clamp(player_.y + 8 - VIEW_H / 2.0f, 0.0f,
                                   static_cast<float>(World::H * TILE - VIEW_H)));
}

bool Game::blockedAt(Vector2 p) const {
    // The player's "feet box" - smaller than the sprite so you can walk close to things.
    const Rectangle hb{p.x + 4, p.y + 7, 8, 8};

    const int x0 = static_cast<int>(std::floor(hb.x / TILE));
    const int x1 = static_cast<int>(std::floor((hb.x + hb.width - 0.01f) / TILE));
    const int y0 = static_cast<int>(std::floor(hb.y / TILE));
    const int y1 = static_cast<int>(std::floor((hb.y + hb.height - 0.01f) / TILE));
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (world_.solid(tx, ty)) return true;

    for (const Npc& n : npcs_)
        if (CheckCollisionRecs(hb, bodyRect(n.pos))) return true;
    if (CheckCollisionRecs(hb, bodyRect(catPos_))) return true;
    return false;
}

void Game::movePlayer(const Input& in, float dt) {
    float mx = in.moveX, my = in.moveY;
    moving_ = (mx != 0 || my != 0);
    if (!moving_) return;

    // Face the direction of the strongest input.
    if (std::fabs(mx) > std::fabs(my)) facing_ = mx > 0 ? Dir::Right : Dir::Left;
    else facing_ = my > 0 ? Dir::Down : Dir::Up;

    // Diagonals shouldn't be faster than straight lines.
    if (mx != 0 && my != 0) {
        mx *= 0.7071f;
        my *= 0.7071f;
    }

    // Move one axis at a time so you slide along walls instead of sticking to them.
    Vector2 next = player_;
    next.x += mx * kWalkSpeed * dt;
    if (!blockedAt(next)) player_.x = next.x;
    next = player_;
    next.y += my * kWalkSpeed * dt;
    if (!blockedAt(next)) player_.y = next.y;

    walkTime_ += dt;
}

void Game::updatePlay(const Input& in, float dt) {
    if (hintTime_ > 0) hintTime_ -= dt;
    movePlayer(in, dt);

    // Pick up mushrooms by walking over them.
    const Vector2 c{player_.x + 8, player_.y + 11};
    for (Mushroom& m : mushrooms_) {
        if (m.taken) continue;
        const Vector2 mc = tileToPixel(m.tile);
        if (std::fabs(c.x - (mc.x + 8)) < 9 && std::fabs(c.y - (mc.y + 11)) < 9) {
            m.taken = true;
            mushroomCount_++;
            const std::string n = std::to_string(mushroomCount_) + "/3";
            if (mushroomCount_ >= 3 && quest_ == Quest::Active) {
                quest_ = Quest::Ready;
                showToast("All 3 found! Take them to Mira.");
            } else {
                showToast("Found a mushroom! (" + n + ")");
            }
        }
    }

    if (in.action) interact();
}

void Game::interact() {
    const Vector2 f = dirVec(facing_);
    const Vector2 c{player_.x + 8, player_.y + 11};

    // Look a little way ahead of the player for something to talk to.
    for (float reach : {8.0f, 14.0f}) {
        const Vector2 pt{c.x + f.x * reach, c.y + f.y * reach};

        for (size_t i = 0; i < npcs_.size(); i++) {
            if (CheckCollisionPointRec(pt, inflate(bodyRect(npcs_[i].pos), 3))) {
                talkToNpc(i);
                return;
            }
        }

        if (CheckCollisionPointRec(pt, inflate(bodyRect(catPos_), 3))) {
            static const char* const meows[] = {"Mew.", "...", "*purr purr*"};
            const std::string line = quest_ == Quest::Done ? "*purr purr*" : meows[catTalks_ % 3];
            catTalks_++;
            startTalk("Cat", {line});
            return;
        }

        const int tx = static_cast<int>(std::floor(pt.x / TILE));
        const int ty = static_cast<int>(std::floor(pt.y / TILE));
        const Tile t = world_.at(tx, ty);
        if (t == Tile::Sign) {
            startTalk("Sign", {"WILLOWBROOK VILLAGE. Population: a few friendly folks and one cat.",
                               "Mira's house is to the west. The pond is to the south-east. "
                               "The forest is to the north."});
            return;
        }
        if (t == Tile::Door) {
            if (tx == 8 && ty == 10) startTalk("", {"Mira's house. Something smells delicious in there."});
            else if (tx == 28) startTalk("", {"A note on the door: 'Gone fishing. Back never.'"});
            else startTalk("", {"Nobody's home. The curtains twitch anyway."});
            return;
        }
    }
}

void Game::talkToNpc(size_t index) {
    Npc& npc = npcs_[index];
    npc.dir = opposite(facing_);  // turn to face whoever is talking to them
    const std::string name = npc.name;

    if (name == "Mira") {
        switch (quest_) {
            case Quest::NotStarted:
                startTalk(name,
                          {"Oh! A new face in Willowbrook. Welcome!",
                           "I'm Mira. I was making mushroom soup, but I've run out of mushrooms.",
                           "Could you find 3 red ones for me? Try the forest edges and near the pond."},
                          [this] {
                              quest_ = mushroomCount_ >= 3 ? Quest::Ready : Quest::Active;
                              showToast("Quest: find 3 mushrooms");
                          });
                break;
            case Quest::Active:
                startTalk(name, {"Any luck with the mushrooms? You have " +
                                     std::to_string(mushroomCount_) + " of 3 so far.",
                                 "They're red with white spots. Look around the forest edges "
                                 "and by the pond."});
                break;
            case Quest::Ready:
                startTalk(name,
                          {"You found all three! You're wonderful.",
                           "Give me a minute... there! Hot mushroom soup, just for you.",
                           "You're welcome in Willowbrook any time. Thank you!"},
                          [this] {
                              quest_ = Quest::Done;
                              showToast("Quest complete: Mushroom Soup!");
                          });
                break;
            case Quest::Done:
                startTalk(name, {"Soup's always on if you're hungry.",
                                 "Look at that sky, though. Willowbrook is lovely at dusk."});
                break;
        }
    } else if (name == "Pip") {
        if (quest_ == Quest::Done)
            startTalk(name, {"Mira's soup again? Lucky you.", "Stay as long as you like, traveler."});
        else
            startTalk(name, {"Hm? Oh, hello. Beautiful day for a walk.",
                             "They say red mushrooms grow where the trees stand thick.",
                             "That orange cat? Nobody knows whose it is. It simply... lives here."});
    }
}

void Game::startTalk(std::string speaker, std::vector<std::string> pages,
                     std::function<void()> onEnd) {
    talk_ = Talk{};
    talk_.speaker = std::move(speaker);
    talk_.pages = std::move(pages);
    talk_.onEnd = std::move(onEnd);
    scene_ = Scene::Talk;
    moving_ = false;
    loadPage();
}

void Game::loadPage() {
    talk_.lines = wrapText(talk_.pages[talk_.page], VIEW_W - 36);
    talk_.totalChars = 0;
    for (const auto& l : talk_.lines) talk_.totalChars += static_cast<int>(l.size());
    talk_.shown = 0;
}

void Game::updateTalk(const Input& in, float dt) {
    talk_.shown += kTypeSpeed * dt;
    if (!in.action) return;

    if (talk_.shown < static_cast<float>(talk_.totalChars)) {
        talk_.shown = static_cast<float>(talk_.totalChars);  // skip the typing effect
    } else if (++talk_.page >= talk_.pages.size()) {
        auto done = std::move(talk_.onEnd);
        scene_ = Scene::Play;
        if (done) done();
    } else {
        loadPage();
    }
}

void Game::showToast(const std::string& text) {
    toast_ = text;
    toastTime_ = 3.0f;
}

// ------------------------------------------------------------------- draw ---

void Game::draw() const {
    ClearBackground(Color{123, 201, 111, 255});
    drawWorld();
    drawActors();

    // After the quest, the whole village slowly glows gold.
    if (dusk_ > 0)
        DrawRectangle(0, 0, VIEW_W, VIEW_H,
                      Color{255, 130, 70, static_cast<unsigned char>(dusk_ * 80)});

    if (scene_ == Scene::Title) {
        drawTitle();
        return;
    }
    drawHud();
    drawToast();
    if (scene_ == Scene::Talk) drawTalk();
}

void Game::drawWorld() const {
    const int camX = static_cast<int>(cam_.x), camY = static_cast<int>(cam_.y);
    const int x0 = camX / TILE, y0 = camY / TILE;
    const int x1 = std::min(World::W - 1, (camX + VIEW_W) / TILE);
    const int y1 = std::min(World::H - 1, (camY + VIEW_H) / TILE);
    const bool waterFrame = static_cast<int>(time_ * 1.6f) % 2 == 1;

    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            Tile t = world_.at(tx, ty);
            if (t == Tile::WaterA && waterFrame) t = Tile::WaterB;
            atlas_.drawTile(t, tx * TILE - camX, ty * TILE - camY, world_.variant(tx, ty));
        }
    }
}

void Game::drawActors() const {
    const int camX = static_cast<int>(cam_.x), camY = static_cast<int>(cam_.y);
    auto sx = [&](float x) { return std::round(x) - camX; };
    auto sy = [&](float y) { return std::round(y) - camY; };

    // Mushrooms bob gently so they catch your eye.
    for (size_t i = 0; i < mushrooms_.size(); i++) {
        const Mushroom& m = mushrooms_[i];
        if (m.taken) continue;
        const Vector2 p = tileToPixel(m.tile);
        const float bob = std::round(std::sin(time_ * 4.0f + static_cast<float>(i)) * 1.5f);
        DrawEllipse(static_cast<int>(sx(p.x)) + 8, static_cast<int>(sy(p.y)) + 14, 4, 2,
                    Fade(BLACK, 0.25f));
        atlas_.drawMushroom(sx(p.x), sy(p.y) + bob);
    }

    // Draw characters from top to bottom so whoever is lower on screen is in front.
    struct Drawable {
        float y;
        int kind;  // 0 = player, 1 = npc, 2 = cat
        size_t idx;
    };
    std::vector<Drawable> order{{player_.y, 0, 0}, {catPos_.y, 2, 0}};
    for (size_t i = 0; i < npcs_.size(); i++) order.push_back({npcs_[i].pos.y, 1, i});
    std::sort(order.begin(), order.end(), [](const Drawable& a, const Drawable& b) { return a.y < b.y; });

    for (const Drawable& d : order) {
        if (d.kind == 0) {
            const int frame = moving_ ? static_cast<int>(walkTime_ * 8.0f) % 2 : 0;
            DrawEllipse(static_cast<int>(sx(player_.x)) + 8, static_cast<int>(sy(player_.y)) + 14, 5, 2,
                        Fade(BLACK, 0.25f));
            atlas_.drawActor(Look::Player, facing_, frame, sx(player_.x), sy(player_.y));
        } else if (d.kind == 1) {
            const Npc& n = npcs_[d.idx];
            DrawEllipse(static_cast<int>(sx(n.pos.x)) + 8, static_cast<int>(sy(n.pos.y)) + 14, 5, 2,
                        Fade(BLACK, 0.25f));
            atlas_.drawActor(n.look, n.dir, 0, sx(n.pos.x), sy(n.pos.y));
        } else {
            const bool blink = std::fmod(time_, 3.4f) > 3.25f;
            DrawEllipse(static_cast<int>(sx(catPos_.x)) + 8, static_cast<int>(sy(catPos_.y)) + 14, 5, 2,
                        Fade(BLACK, 0.25f));
            atlas_.drawCat(blink, sx(catPos_.x), sy(catPos_.y));
        }
    }
}

void Game::drawHud() const {
    if (quest_ == Quest::Active || quest_ == Quest::Ready) {
        DrawRectangle(4, 4, 44, 16, Color{30, 22, 38, 200});
        atlas_.drawMushroomIcon(7, 6, 12);
        drawText(std::to_string(mushroomCount_) + "/3", 23, 8, kTextSize,
                 mushroomCount_ >= 3 ? kGold : kCream);
    }
    if (hintTime_ > 0 && scene_ == Scene::Play) {
        const unsigned char a = static_cast<unsigned char>(255 * std::min(1.0f, hintTime_));
        const std::string hint = "Arrows / WASD: move     Z / Space: talk";
        const int w = MeasureText(hint.c_str(), kTextSize) + 14;
        DrawRectangle(VIEW_W / 2 - w / 2, VIEW_H - 22, w, 16, Color{30, 22, 38, static_cast<unsigned char>(a * 0.8f)});
        DrawText(hint.c_str(), VIEW_W / 2 - w / 2 + 7, VIEW_H - 18, kTextSize, Color{246, 240, 228, a});
    }
}

void Game::drawToast() const {
    if (toastTime_ <= 0) return;
    const unsigned char a = static_cast<unsigned char>(255 * std::min(1.0f, toastTime_ / 0.5f));
    const int w = MeasureText(toast_.c_str(), kTextSize) + 16;
    DrawRectangle(VIEW_W / 2 - w / 2, 24, w, 16, Color{30, 22, 38, static_cast<unsigned char>(a * 0.9f)});
    DrawText(toast_.c_str(), VIEW_W / 2 - w / 2 + 8, 28, kTextSize, Color{243, 213, 91, a});
}

void Game::drawTalk() const {
    const Rectangle box{6, 170, VIEW_W - 12, 64};
    DrawRectangleRec(box, kPanel);
    DrawRectangleLinesEx(box, 2, kCream);

    if (!talk_.speaker.empty()) {
        const int w = MeasureText(talk_.speaker.c_str(), kTextSize) + 14;
        DrawRectangle(14, 160, w, 16, kCream);
        DrawText(talk_.speaker.c_str(), 21, 164, kTextSize, kInk);
    }

    int remaining = static_cast<int>(talk_.shown);
    for (size_t i = 0; i < talk_.lines.size() && remaining > 0; i++) {
        const std::string& line = talk_.lines[i];
        const int n = std::min(remaining, static_cast<int>(line.size()));
        DrawText(line.substr(0, static_cast<size_t>(n)).c_str(), 16, 182 + static_cast<int>(i) * 13,
                 kTextSize, kCream);
        remaining -= n;
    }

    // A little blinking arrow when there's more to read.
    if (talk_.shown >= static_cast<float>(talk_.totalChars) && std::fmod(time_, 0.9f) < 0.55f) {
        for (int i = 0; i < 5; i++) DrawRectangle(298 + i, 222 + i, 9 - i * 2, 1, kGold);
    }
}

void Game::drawTitle() const {
    DrawRectangle(0, 0, VIEW_W, VIEW_H, Color{20, 16, 40, 100});

    const Rectangle card{40, 46, VIEW_W - 80, 82};
    DrawRectangleRec(card, kPanel);
    DrawRectangleLinesEx(card, 2, kCream);
    drawText("WILLOWBROOK", VIEW_W / 2, 62, 20, kGold, true);
    drawText("a cozy little village", VIEW_W / 2, 92, kTextSize, kCream, true);
    drawText("made with C++ and raylib", VIEW_W / 2, 108, kTextSize, Color{190, 180, 200, 255}, true);

    if (std::fmod(time_, 1.2f) < 0.8f)
        drawText("Press ENTER or SPACE", VIEW_W / 2, 152, kTextSize, kCream, true);
    drawText("Arrows / WASD: move     Z / Space: talk", VIEW_W / 2, 218, kTextSize,
             Color{220, 214, 224, 255}, true);
}

}  // namespace cozy
