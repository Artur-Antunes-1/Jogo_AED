#include "game.h"

void init_default_colors(AppColors* colors) {
    if (!colors) {
        return;
    }

    *colors = (AppColors){
        .bgColor = (SDL_Color){30, 30, 40, 255},
        .bgGradientEnd = (SDL_Color){15, 15, 20, 255},
        .textColor = (SDL_Color){230, 230, 230, 255},
        .titleColor = (SDL_Color){255, 200, 0, 255},
        .buttonColor = (SDL_Color){70, 70, 90, 255},
        .highlightColor = (SDL_Color){100, 100, 130, 255},
        .inputBgColor = (SDL_Color){50, 50, 60, 255},
        .accentGreen = (SDL_Color){50, 200, 50, 255},
        .accentRed = (SDL_Color){200, 50, 50, 255},
        .accentGray = (SDL_Color){150, 150, 150, 255}
    };
}

