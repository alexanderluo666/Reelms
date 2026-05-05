#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

enum GameState { MENU, PLAYING, UPGRADE, GAME_OVER };
enum UpgradeType { FIRE_RATE, MULTISHOT, SPEED };
enum EnemyType { NORMAL, FAST, TANK, DASHER, BOSS, MEGA_BOSS, GOD_BOSS };

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
    int type;
    int hp;
    bool alive;
};

struct Laser {
    Vector2 pos;
    Vector2 dir;
    float speed;
    float radius;
    bool alive;
};

float Dist(Vector2 a, Vector2 b) {
    return sqrtf((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

Vector2 GetSafeSpawn(Vector2 playerPos, float minDist) {
    Vector2 pos;
    do {
        pos = {(float)GetRandomValue(0,800),(float)GetRandomValue(0,600)};
    } while (Dist(pos, playerPos) < minDist);
    return pos;
}

const char* GetUpgradeName(UpgradeType u) {
    switch(u) {
        case FIRE_RATE: return "Faster Fire";
        case MULTISHOT: return "Multi Shot";
        case SPEED: return "Move Speed";
    }
    return "";
}

int main() {
    InitWindow(800, 600, "REELMS: Riftfall");
    SetTargetFPS(60);

    GameState state = MENU;

    Player player;
    std::vector<Enemy> enemies;
    std::vector<Laser> lasers;

    int wave = 1;
    int enemiesAlive = 0;
    int killCount = 0;

    float rareChance = 1.1f;
    UpgradeType choices[3];

    Camera2D cam = {0};
    cam.zoom = 1.0f;

    auto ResetGame = [&]() {
        player = {{400,300}, 250, 12, 5, 0.4f, 0, 1};
        enemies.clear();
        lasers.clear();
        wave = 1;
        enemiesAlive = 0;
        killCount = 0;
        rareChance = 1.1f;
    };

    auto SpawnWave = [&]() {

        if (wave % 100 == 0) {
            enemies.push_back({{400,100},60,40,GOD_BOSS,50,true});
            enemiesAlive = 1;
        }
        else if (wave % 50 == 0) {
            enemies.push_back({{400,100},70,30,MEGA_BOSS,25,true});
            enemiesAlive = 1;
        }
        else if (wave % 10 == 0) {
            enemies.push_back({{400,100},90,25,BOSS,12,true});
            enemiesAlive = 1;
        }
        else {
            int count = 5 + wave * 2;
            for (int i=0;i<count;i++) {
                Enemy e;
                e.pos = GetSafeSpawn(player.pos, 10.0f);

                int r = GetRandomValue(0,100);
                if (r<50) e.type=NORMAL;
                else if (r<75) e.type=FAST;
                else if (r<90) e.type=TANK;
                else e.type=DASHER;

                e.speed = (e.type==FAST)?140:(e.type==TANK?50:80);
                e.radius = (e.type==TANK)?16:10;
                e.hp = 1;
                e.alive = true;

                enemies.push_back(e);
            }
            enemiesAlive = count;
        }
        wave++;
    };

    auto RollUpgrades = [&]() {
        for (int i=0;i<3;i++)
            choices[i]=(UpgradeType)GetRandomValue(0,2);
    };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (state == MENU) {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
                state = PLAYING;
            }
        }

        else if (state == PLAYING) {

            if (enemiesAlive == 0)
                SpawnWave();

            Vector2 dir={0,0};
            if (IsKeyDown(KEY_W)) dir.y--;
            if (IsKeyDown(KEY_S)) dir.y++;
            if (IsKeyDown(KEY_A)) dir.x--;
            if (IsKeyDown(KEY_D)) dir.x++;

            if (dir.x||dir.y) {
                float len=sqrtf(dir.x*dir.x+dir.y*dir.y);
                dir.x/=len; dir.y/=len;
                player.pos.x+=dir.x*player.speed*dt;
                player.pos.y+=dir.y*player.speed*dt;
            }

            player.fireCooldown -= dt;
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && player.fireCooldown<=0) {
                player.fireCooldown=player.fireRate;

                Vector2 m = GetScreenToWorld2D(GetMousePosition(), cam);

                for(int i=0;i<player.multiShot;i++){
                    Vector2 d={m.x-player.pos.x,m.y-player.pos.y};
                    float len=sqrtf(d.x*d.x+d.y*d.y);
                    if(len){d.x/=len;d.y/=len;}
                    lasers.push_back({player.pos,d,500,5,true});
                }
            }

            for(auto &e:enemies){
                if(!e.alive) continue;

                Vector2 d={player.pos.x-e.pos.x,player.pos.y-e.pos.y};
                float len=sqrtf(d.x*d.x+d.y*d.y);
                if(len){d.x/=len;d.y/=len;}

                if(e.type==DASHER && GetRandomValue(0,100)<2){
                    e.pos.x+=d.x*200;
                    e.pos.y+=d.y*200;
                }

                e.pos.x+=d.x*e.speed*dt;
                e.pos.y+=d.y*e.speed*dt;

                if(Dist(player.pos,e.pos)<player.radius+e.radius){
                    player.hp--;
                    e.alive=false;
                    enemiesAlive--;
                }
            }

            for(auto &l:lasers){
                if(!l.alive) continue;

                l.pos.x+=l.dir.x*l.speed*dt;
                l.pos.y+=l.dir.y*l.speed*dt;

                for(auto &e:enemies){
                    if(!e.alive) continue;

                    if(Dist(l.pos,e.pos)<l.radius+e.radius){
                        l.alive=false;
                        e.hp--;

                        if(e.hp<=0){
                            e.alive=false;
                            enemiesAlive--;
                            killCount++;
                        }
                    }
                }
            }

            // cleanup
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                [](Enemy &e){ return !e.alive; }), enemies.end());

            lasers.erase(std::remove_if(lasers.begin(), lasers.end(),
                [](Laser &l){ return !l.alive; }), lasers.end());

            if (killCount>0 && killCount%10==0){
                float roll=GetRandomValue(0,10000)/100.0f;

                if(roll<rareChance){
                    player.multiShot+=2;
                    player.fireRate*=0.7f;
                    rareChance=1.1f;
                } else {
                    rareChance*=1.5f;
                }

                RollUpgrades();
                state=UPGRADE;
                killCount++;
            }

            if(player.hp<=0)
                state=GAME_OVER;
        }

        else if(state==UPGRADE){
            if(IsKeyPressed(KEY_ONE)) state=PLAYING;
            if(IsKeyPressed(KEY_TWO)) state=PLAYING;
            if(IsKeyPressed(KEY_THREE)) state=PLAYING;
        }

        else if(state==GAME_OVER){
            if(IsKeyPressed(KEY_ENTER)) state=MENU;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if(state==MENU){
            DrawText("REELMS: RIFTFALL",200,200,40,WHITE);
            DrawText("PRESS ENTER",300,300,20,GRAY);
        }

        else if(state==PLAYING){
            cam.target = player.pos;
            cam.offset = {400,300};

            BeginMode2D(cam);

            DrawCircleV(player.pos,player.radius,GREEN);

            for(auto &e:enemies)
                DrawCircleV(e.pos,e.radius,(e.type>=BOSS)?PURPLE:RED);

            for(auto &l:lasers)
                DrawCircleV(l.pos,l.radius,YELLOW);

            EndMode2D();

            DrawText(TextFormat("HP:%d",player.hp),10,10,20,WHITE);
            DrawText(TextFormat("Wave:%d",wave),10,40,20,WHITE);
        }

        else if(state==UPGRADE){
            DrawText("CHOOSE UPGRADE",220,100,30,YELLOW);
            for(int i=0;i<3;i++){
                DrawText(GetUpgradeName(choices[i]),200+i*180,250,20,WHITE);
                DrawText(TextFormat("[%d]",i+1),250+i*180,300,20,GRAY);
            }
        }

        else if(state==GAME_OVER){
            DrawText("YOU DIED",300,200,40,RED);
            DrawText("ENTER TO RESTART",250,300,20,GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
}
