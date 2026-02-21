#include "types.h"
#include "game.h"
#include <raylib.h>

int main(void) {
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    InitWindow(screenWidth, screenHeight, "Vortex Clash");

    GameState game;
    game_init(&game);

    /* Fixed timestep: 60 updates per second */
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

        /* Handle input (sampled every frame for responsiveness) */
        /* Update at fixed timestep */
        while (accumulator >= updateInterval) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.running = FALSE;
                break;
            }
            game_update(&game);
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
