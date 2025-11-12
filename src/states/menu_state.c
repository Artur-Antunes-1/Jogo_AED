#include "menu_state.h"

#include <SDL3/SDL.h>

#include "render_utils.h"
#include "text_utils.h"

GameState runMenu(GameContext* context) {
    if (!context) {
        return STATE_EXIT;
    }

    SDL_Renderer* renderer = context->renderer;

    int selectedOption = 0;
    const int numOptions = 4;
    const int buttonWidth = 400;
    const int buttonHeight = 60;
    const int buttonYStart = 250;
    const int buttonPadding = 20;

    SDL_Color buttonColor = context->colors.buttonColor;
    SDL_Color selectedColor = context->colors.highlightColor;
    SDL_Color textColor = context->colors.textColor;
    SDL_Color textSelectedColor = context->colors.titleColor;
    SDL_Color titleColor = context->colors.titleColor;

    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    createTextTexture(context, 1, "Jogo de Adedonha (STOP!)", &titleTexture, &titleRect, 0, 100, titleColor);

    SDL_Texture* playTextures[2]; SDL_FRect playRects[2];
    SDL_Texture* boardTextures[2]; SDL_FRect boardRects[2];
    SDL_Texture* optionsTextures[2]; SDL_FRect optionsRects[2];
    SDL_Texture* exitTextures[2]; SDL_FRect exitRects[2];

    createTextTexture(context, 0, "Iniciar Jogo", &playTextures[0], &playRects[0], 0, 0, textColor);
    createTextTexture(context, 0, "Iniciar Jogo", &playTextures[1], &playRects[1], 0, 0, textSelectedColor);

    createTextTexture(context, 0, "Ver Placar", &boardTextures[0], &boardRects[0], 0, 0, textColor);
    createTextTexture(context, 0, "Ver Placar", &boardTextures[1], &boardRects[1], 0, 0, textSelectedColor);

    createTextTexture(context, 0, "Opções de Letras", &optionsTextures[0], &optionsRects[0], 0, 0, textColor);
    createTextTexture(context, 0, "Opções de Letras", &optionsTextures[1], &optionsRects[1], 0, 0, textSelectedColor);

    createTextTexture(context, 0, "Sair", &exitTextures[0], &exitRects[0], 0, 0, textColor);
    createTextTexture(context, 0, "Sair", &exitTextures[1], &exitRects[1], 0, 0, textSelectedColor);

    SDL_FRect playButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart, buttonWidth, buttonHeight };
    SDL_FRect boardButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart + (buttonHeight + buttonPadding), buttonWidth, buttonHeight };
    SDL_FRect optionsButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart + (buttonHeight + buttonPadding) * 2, buttonWidth, buttonHeight };
    SDL_FRect exitButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart + (buttonHeight + buttonPadding) * 3, buttonWidth, buttonHeight };

    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2.0f;
    playRects[0].x = playRects[1].x = (SCREEN_WIDTH - playRects[0].w) / 2.0f;
    playRects[0].y = playRects[1].y = playButtonRect.y + (buttonHeight - playRects[0].h) / 2.0f;
    boardRects[0].x = boardRects[1].x = (SCREEN_WIDTH - boardRects[0].w) / 2.0f;
    boardRects[0].y = boardRects[1].y = boardButtonRect.y + (buttonHeight - boardRects[0].h) / 2.0f;
    optionsRects[0].x = optionsRects[1].x = (SCREEN_WIDTH - optionsRects[0].w) / 2.0f;
    optionsRects[0].y = optionsRects[1].y = optionsButtonRect.y + (buttonHeight - optionsRects[0].h) / 2.0f;
    exitRects[0].x = exitRects[1].x = (SCREEN_WIDTH - exitRects[0].w) / 2.0f;
    exitRects[0].y = exitRects[1].y = exitButtonRect.y + (buttonHeight - exitRects[0].h) / 2.0f;

    int running_menu = 1;
    SDL_Event event;

    while (running_menu) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running_menu = 0;
                selectedOption = 3; // Exit
                break;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_UP:
                        selectedOption = (selectedOption - 1 + numOptions) % numOptions;
                        break;
                    case SDLK_DOWN:
                        selectedOption = (selectedOption + 1) % numOptions;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        running_menu = 0;
                        break;
                }
            }
        }

        drawGradientBackground(renderer, context->colors.bgColor, context->colors.bgGradientEnd);

        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);

        SDL_Color playColor = (selectedOption == 0) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, playColor.r, playColor.g, playColor.b, playColor.a);
        SDL_RenderFillRect(renderer, &playButtonRect);
        SDL_RenderTexture(renderer, playTextures[selectedOption == 0], NULL, &playRects[selectedOption == 0]);
        if (selectedOption == 0) {
            SDL_SetRenderDrawColor(renderer, textSelectedColor.r, textSelectedColor.g, textSelectedColor.b, 255);
            SDL_RenderRect(renderer, &playButtonRect);
        }

        SDL_Color boardColor = (selectedOption == 1) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, boardColor.r, boardColor.g, boardColor.b, boardColor.a);
        SDL_RenderFillRect(renderer, &boardButtonRect);
        SDL_RenderTexture(renderer, boardTextures[selectedOption == 1], NULL, &boardRects[selectedOption == 1]);
        if (selectedOption == 1) {
            SDL_SetRenderDrawColor(renderer, textSelectedColor.r, textSelectedColor.g, textSelectedColor.b, 255);
            SDL_RenderRect(renderer, &boardButtonRect);
        }

        SDL_Color optionsColor = (selectedOption == 2) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, optionsColor.r, optionsColor.g, optionsColor.b, optionsColor.a);
        SDL_RenderFillRect(renderer, &optionsButtonRect);
        SDL_RenderTexture(renderer, optionsTextures[selectedOption == 2], NULL, &optionsRects[selectedOption == 2]);
        if (selectedOption == 2) {
            SDL_SetRenderDrawColor(renderer, textSelectedColor.r, textSelectedColor.g, textSelectedColor.b, 255);
            SDL_RenderRect(renderer, &optionsButtonRect);
        }

        SDL_Color exitColor = (selectedOption == 3) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, exitColor.r, exitColor.g, exitColor.b, exitColor.a);
        SDL_RenderFillRect(renderer, &exitButtonRect);
        SDL_RenderTexture(renderer, exitTextures[selectedOption == 3], NULL, &exitRects[selectedOption == 3]);
        if (selectedOption == 3) {
            SDL_SetRenderDrawColor(renderer, textSelectedColor.r, textSelectedColor.g, textSelectedColor.b, 255);
            SDL_RenderRect(renderer, &exitButtonRect);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(titleTexture);
    for (int i = 0; i < 2; i++) {
        SDL_DestroyTexture(playTextures[i]);
        SDL_DestroyTexture(boardTextures[i]);
        SDL_DestroyTexture(optionsTextures[i]);
        SDL_DestroyTexture(exitTextures[i]);
    }

    switch (selectedOption) {
        case 0: return STATE_PLAYING;
        case 1: return STATE_LEADERBOARD;
        case 2: return STATE_OPTIONS;
        case 3: default: return STATE_EXIT;
    }
}

