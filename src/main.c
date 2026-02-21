#include "types.h"
#include "game.h"
#include <raylib.h>
#include <stdlib.h>

int main(void) {
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    /* Game speed: set VORTEX_SPEED env var (default 1, try 2 for 2x, 3 for 3x) */
    int game_speed = 1;
    const char *speed_env = getenv("VORTEX_SPEED");
    if (speed_env) {
        int s = atoi(speed_env);
        if (s >= 1 && s <= 4) game_speed = s;
    }

    InitWindow(screenWidth, screenHeight, "Vortex Clash");

    GameState game;
    game_init(&game);

    /* Fixed timestep: locked 60fps */
    const int targetFPS = 60;
    const double updateInterval = 1.0 / targetFPS;
    double accumulator = 0.0;
    double currentTime = GetTime();

    SetTargetFPS(targetFPS);

    while (!WindowShouldClose() && game.running) {
        double newTime = GetTime();
        double frameTime = newTime - currentTime;
        currentTime = newTime;

        accumulator += frameTime;

        /* Update at fixed 60fps timestep, multiple ticks for speed */
        while (accumulator >= updateInterval) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.running = FALSE;
                break;
            }
            for (int i = 0; i < game_speed; i++) {
                game_update(&game);
            }
            accumulator -= updateInterval;
        }

        /* Render */
        BeginDrawing();
        game_render(&game);
        EndDrawing();
    }

    game_shutdown(&game);
    CloseWindow();
    return 0;
}
