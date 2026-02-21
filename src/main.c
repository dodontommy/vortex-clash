#include "types.h"
#include <raylib.h>

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Vortex Clash");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Vortex Clash", screenWidth / 2 - 120, screenHeight / 2 - 20, 40, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
