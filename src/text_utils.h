#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include "game.h"

void createTextTexture(GameContext* context,
                       int isTitleFont,
                       const char* text,
                       SDL_Texture** texture,
                       SDL_FRect* rect,
                       int x,
                       int y,
                       SDL_Color color);

#endif /* TEXT_UTILS_H */

