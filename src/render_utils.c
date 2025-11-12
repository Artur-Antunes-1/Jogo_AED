#include "render_utils.h"

#include "game.h"

void drawGradientBackground(SDL_Renderer* renderer, SDL_Color start, SDL_Color end) {
    if (!renderer) {
        return;
    }

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float ratio = (float)y / (float)SCREEN_HEIGHT;
        Uint8 r = (Uint8)(start.r * (1.0f - ratio) + end.r * ratio);
        Uint8 g = (Uint8)(start.g * (1.0f - ratio) + end.g * ratio);
        Uint8 b = (Uint8)(start.b * (1.0f - ratio) + end.b * ratio);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
}

