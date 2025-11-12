#include "text_utils.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>

#include <string.h>

void createTextTexture(GameContext* context,
                       int isTitleFont,
                       const char* text,
                       SDL_Texture** texture,
                       SDL_FRect* rect,
                       int x,
                       int y,
                       SDL_Color color) {
    if (!context || !rect || !texture) {
        return;
    }

    if (*texture) {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }

    if (!text || text[0] == '\0') {
        rect->w = 0;
        rect->h = 0;
        return;
    }

    TTF_Font* fontToUse = isTitleFont ? context->font_title : context->font_body;
    SDL_Surface* textSurface = TTF_RenderText_Blended(fontToUse, text, (int)strlen(text), color);
    if (!textSurface) {
        SDL_Log("Erro ao criar superficie de texto: %s", SDL_GetError());
        return;
    }

    *texture = SDL_CreateTextureFromSurface(context->renderer, textSurface);
    rect->x = (float)x;
    rect->y = (float)y;
    rect->w = (float)textSurface->w;
    rect->h = (float)textSurface->h;
    SDL_DestroySurface(textSurface);
}

