#include "options_state.h"

#include <SDL3/SDL.h>

#include <stdio.h>

#include "render_utils.h"
#include "text_utils.h"

GameState runOptions(GameContext* context) {
    if (!context) {
        return STATE_EXIT;
    }

    SDL_Renderer* renderer = context->renderer;

    SDL_Color titleColor = context->colors.titleColor;
    SDL_Color enabledColor = context->colors.accentGreen;
    SDL_Color disabledColor = context->colors.accentRed;
    SDL_Color selectedColor = context->colors.textColor;
    SDL_Color textColor = context->colors.accentGray;

    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    createTextTexture(context, 1, "Opções de Letras", &titleTexture, &titleRect, 0, 50, titleColor);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;

    SDL_Texture* helpTexture = NULL;
    SDL_FRect helpRect;
    createTextTexture(context, 0, "Use Setas para Mover, ENTER para Ativar/Desativar, ESC para Voltar", &helpTexture, &helpRect, 0, 650, textColor);
    helpRect.x = (SCREEN_WIDTH - helpRect.w) / 2;

    int selectedLetter = 0;

    const int numCols = 6;
    const int spacingX = 150;
    const int spacingY = 80;
    const int startX = (SCREEN_WIDTH - (spacingX * (numCols - 1))) / 2;
    const int startY = 150;

    int running_options = 1;
    SDL_Event event;

    while (running_options) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                SDL_DestroyTexture(titleTexture);
                SDL_DestroyTexture(helpTexture);
                return STATE_EXIT;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_ESCAPE:
                        running_options = 0;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        context->isLetterEnabled[selectedLetter] = !context->isLetterEnabled[selectedLetter];
                        break;
                    case SDLK_RIGHT:
                        if ((selectedLetter + 1) % numCols == 0) {
                            selectedLetter -= (numCols - 1);
                        } else if (selectedLetter + 1 < 26) {
                            selectedLetter++;
                        }
                        break;
                    case SDLK_LEFT:
                        if (selectedLetter % numCols == 0) {
                            selectedLetter += (numCols - 1);
                        } else {
                            selectedLetter--;
                        }
                        if (selectedLetter > 25) {
                            selectedLetter = 25;
                        }
                        break;
                    case SDLK_DOWN:
                        if (selectedLetter + numCols < 26) {
                            selectedLetter += numCols;
                        }
                        break;
                    case SDLK_UP:
                        if (selectedLetter - numCols >= 0) {
                            selectedLetter -= numCols;
                        }
                        break;
                }
            }
        }

        drawGradientBackground(renderer, context->colors.bgColor, context->colors.bgGradientEnd);

        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderTexture(renderer, helpTexture, NULL, &helpRect);

        for (int i = 0; i < 26; i++) {
            int row = i / numCols;
            int col = i % numCols;
            int xPos = startX + (col * spacingX);
            int yPos = startY + (row * spacingY);

            char letterChar = 'A' + i;
            const char* status = context->isLetterEnabled[i] ? "[ON]" : "[OFF]";
            SDL_Color statusColor = context->isLetterEnabled[i] ? enabledColor : disabledColor;

            char entryText[20];
            sprintf(entryText, "%c: %s", letterChar, status);

            SDL_Texture* entryTexture = NULL;
            SDL_FRect entryRect;
            createTextTexture(context, 0, entryText, &entryTexture, &entryRect, xPos, yPos, statusColor);

            if (i == selectedLetter) {
                SDL_FRect cursorRect = {entryRect.x - 10, entryRect.y - 5,
                                        entryRect.w + 20, entryRect.h + 10};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, selectedColor.r, selectedColor.g, selectedColor.b, 100);
                SDL_RenderFillRect(renderer, &cursorRect);
                SDL_RenderRect(renderer, &cursorRect);
            }

            SDL_RenderTexture(renderer, entryTexture, NULL, &entryRect);
            SDL_DestroyTexture(entryTexture);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(helpTexture);

    return STATE_MENU;
}

