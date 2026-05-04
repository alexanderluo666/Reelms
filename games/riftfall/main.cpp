#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

enum GameState { MENU, PLAYING, UPGRADE, GAME_OVER };
enum UpgradeType { FIRE_RATE, MULTISHOT, SPEED };

struct Player {
    Vector2 pos;
    float speed;
    float radius;
    int hp;

    float fireRate;
    float fireCooldown;
    int multiShot;
};

struct Enemy {
    Vector2 pos;
    float speed;
    float radius;
    int type; // 0 normal, 1 fast, 2 tank
    bool alive;
};

struct Laser {
    Vector2 pos;
    Vector2 dir;
    float speed;
    float radius;
    bool alive;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
};

float Dist(Vector2 a, Vector2 b) {
    return sqrtf((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

int main() {
    InitWindow(800, 600, "REELMS: Riftfall");
    SetTargetFPS(60);

    GameState state = MENU;

    Player player;
    std::vector<Enemy> enemies;
    std::vector<Laser> lasers;
    std::vector<Particle> particles;

    int wave = 1;
    int enemiesAlive = 0;
    int killCount = 0;

    float shakeTime = 0;

    // upgrade choice system
    UpgradeType choices[3];

    Camera2D cam = {0};
    cam.zoom = 1.0f;

    auto ResetGame = [&]() {
        player = {{400,300}, 250.0f, 12.0f, 5, 0.4f, 0, 1};
        enemies.clear();
        lasers.clear();
        particles.clear();
        wave = 1;
        enemiesAlive = 0;
        killCount = 0;
    };

    auto SpawnWave = [&]() {
        int count = 5 + wave * 2;

        for (int i = 0; i < count; i++) {
            Enemy e;
            e.pos = {(float)GetRandomValue(0,800),(float)GetRandomValue(0,600)};
            e.type = GetRandomValue(0,100) < 70 ? 0 : (GetRandomValue(0,1) ? 1 : 2);

            if (e.type == 0) e.speed = 80;
            if (e.type == 1) e.speed = 140;
            if (e.type == 2) e.speed = 50;

            e.radius = (e.type == 2) ? 16 : 10;
            e.alive = true;

            enemies.push_back(e);
        }

        enemiesAlive = count;
        wave++;
    };

    auto RollUpgrades = [&]() {
        for (int i = 0; i < 3; i++) {
            choices[i] = (UpgradeType)GetRandomValue(0,2);
        }
    };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ================= UPDATE =================

        if (state == MENU) {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
                state = PLAYING;
            }
        }

        else if (state == PLAYING) {

            // spawn waves
            if (enemiesAlive == 0) {
                SpawnWave();
            }

            // player movement
            Vector2 dir = {0,0};
            if (IsKeyDown(KEY_W)) dir.y -= 1;
            if (IsKeyDown(KEY_S)) dir.y += 1;
            if (IsKeyDown(KEY_A)) dir.x -= 1;
            if (IsKeyDown(KEY_D)) dir.x += 1;

            if (dir.x || dir.y) {
                float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
                dir.x/=len; dir.y/=len;
                player.pos.x += dir.x * player.speed * dt;
                player.pos.y += dir.y * player.speed * dt;
            }

            // shooting
            player.fireCooldown -= dt;
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && player.fireCooldown <= 0) {
                player.fireCooldown = player.fireRate;

                Vector2 mouse = GetMousePosition();

                for (int i = 0; i < player.multiShot; i++) {
                    Vector2 d = {mouse.x-player.pos.x, mouse.y-player.pos.y};
                    float len = sqrtf(d.x*d.x + d.y*d.y);
                    if (len) { d.x/=len; d.y/=len; }

                    d.x += GetRandomValue(-10,10)/100.0f;
                    d.y += GetRandomValue(-10,10)/100.0f;

                    lasers.push_back({player.pos, d, 500, 5, true});
                }
            }

            // enemies
            for (auto &e : enemies) {
                if (!e.alive) continue;

                Vector2 d = {player.pos.x-e.pos.x, player.pos.y-e.pos.y};
                float len = sqrtf(d.x*d.x + d.y*d.y);
                if (len) { d.x/=len; d.y/=len; }

                e.pos.x += d.x * e.speed * dt;
                e.pos.y += d.y * e.speed * dt;

                if (Dist(player.pos, e.pos) < player.radius + e.radius) {
                    player.hp--;
                    e.alive = false;
                    enemiesAlive--;
                    shakeTime = 0.2f;
                }
            }

            // lasers
            for (auto &l : lasers) {
                if (!l.alive) continue;

                l.pos.x += l.dir.x * l.speed * dt;
                l.pos.y += l.dir.y * l.speed * dt;
            }

            // collisions
            for (auto &l : lasers) {
                if (!l.alive) continue;

                for (auto &e : enemies) {
                    if (!e.alive) continue;

                    if (Dist(l.pos, e.pos) < l.radius + e.radius) {
                        l.alive = false;
                        e.alive = false;
                        enemiesAlive--;
                        killCount++;
                        shakeTime = 0.1f;

                        for (int i=0;i<8;i++) {
                            particles.push_back({e.pos,
                                {(float)GetRandomValue(-100,100),
                                 (float)GetRandomValue(-100,100)},
                                0.4f});
                        }
                    }
                }
            }

            // upgrade trigger
            if (killCount > 0 && killCount % 10 == 0) {
                RollUpgrades();
                state = UPGRADE;
                killCount++;
            }

            if (player.hp <= 0) state = GAME_OVER;
        }

        else if (state == UPGRADE) {

            if (IsKeyPressed(KEY_ONE)) {
                if (choices[0] == FIRE_RATE) player.fireRate *= 0.85f;
                if (choices[0] == MULTISHOT) player.multiShot++;
                if (choices[0] == SPEED) player.speed += 20;
                state = PLAYING;
            }

            if (IsKeyPressed(KEY_TWO)) {
                if (choices[1] == FIRE_RATE) player.fireRate *= 0.85f;
                if (choices[1] == MULTISHOT) player.multiShot++;
                if (choices[1] == SPEED) player.speed += 20;
                state = PLAYING;
            }

            if (IsKeyPressed(KEY_THREE)) {
                if (choices[2] == FIRE_RATE) player.fireRate *= 0.85f;
                if (choices[2] == MULTISHOT) player.multiShot++;
                if (choices[2] == SPEED) player.speed += 20;
                state = PLAYING;
            }
        }

        else if (state == GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER)) state = MENU;
        }

        // particles
        for (auto &p : particles) {
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.life -= dt;
        }

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](Enemy &e){ return !e.alive; }), enemies.end());

        lasers.erase(std::remove_if(lasers.begin(), lasers.end(),
            [](Laser &l){ return !l.alive; }), lasers.end());

        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](Particle &p){ return p.life <= 0; }), particles.end());

        // ================= DRAW =================

        BeginDrawing();

        Vector2 shake = {0,0};
        if (shakeTime > 0) {
            shake = {(float)GetRandomValue(-5,5),(float)GetRandomValue(-5,5)};
            shakeTime -= dt;
        }

        ClearBackground(BLACK);

        if (state == MENU) {
            DrawText("REELMS: RIFTFALL", 200,200,40,WHITE);
            DrawText("ENTER TO START", 280,300,20,GRAY);
        }

        else if (state == PLAYING) {

            cam.target = player.pos;
            cam.offset = {400 + shake.x, 300 + shake.y};

            BeginMode2D(cam);

            DrawCircleV(player.pos, player.radius, GREEN);

            for (auto &e : enemies)
                DrawCircleV(e.pos, e.radius,
                    e.type==0?RED:(e.type==1?BLUE:ORANGE));

            for (auto &l : lasers)
                DrawCircleV(l.pos, l.radius, YELLOW);

            for (auto &p : particles)
                DrawCircleV(p.pos, 2, WHITE);

            EndMode2D();

            DrawText(TextFormat("HP:%d", player.hp), 10,10,20,WHITE);
            DrawText(TextFormat("WAVE:%d", wave), 10,40,20,WHITE);
        }

        else if (state == UPGRADE) {
            DrawText("UPGRADE!", 320,100,40,YELLOW);

            DrawText("1", 200,250,30,WHITE);
            DrawText("2", 350,250,30,WHITE);
            DrawText("3", 500,250,30,WHITE);

            DrawText("Press 1/2/3", 300,400,20,GRAY);
        }

        else if (state == GAME_OVER) {
            DrawText("YOU DIED", 300,200,40,RED);
            DrawText("ENTER TO RESTART", 250,300,20,GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
}
