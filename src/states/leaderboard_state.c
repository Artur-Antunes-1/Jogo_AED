#include "leaderboard_state.h"

#include <SDL3/SDL.h>

#include <stdio.h>

#include "leaderboard.h"
#include "render_utils.h"
#include "text_utils.h"

GameState runLeaderboard(GameContext* context) {
    if (!context) {
        return STATE_EXIT;
    }

    SDL_Renderer* renderer = context->renderer;
    SDL_Color white = context->colors.textColor;
    SDL_Color yellow = context->colors.titleColor;

    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    createTextTexture(context, 1, "Placar de Líderes", &titleTexture, &titleRect, 0, 100, yellow);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;

    SDL_Texture* subtitleTexture = NULL;
    SDL_FRect subtitleRect;
    createTextTexture(context, 0, "Pressione ESC para voltar", &subtitleTexture, &subtitleRect, 0, 650, white);
    subtitleRect.x = (SCREEN_WIDTH - subtitleRect.w) / 2;

    int running_leaderboard = 1;
    SDL_Event event;
    while (running_leaderboard) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                SDL_DestroyTexture(titleTexture);
                SDL_DestroyTexture(subtitleTexture);
                return STATE_EXIT;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running_leaderboard = 0;
                }
            }
        }

        drawGradientBackground(renderer, context->colors.bgColor, context->colors.bgGradientEnd);

        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderTexture(renderer, subtitleTexture, NULL, &subtitleRect);

        PlayerNode* current = context->leaderboard;
        int yPos = 250;
        int rank = 1;
        while (current != NULL && rank <= 5) {
            char scoreEntry[100];
            sprintf(scoreEntry, "%d. %s - %d Pontos", rank, current->name, current->totalScore);
            SDL_Texture* entryTexture = NULL;
            SDL_FRect entryRect;
            createTextTexture(context, 0, scoreEntry, &entryTexture, &entryRect, 0, yPos, white);
            entryRect.x = (SCREEN_WIDTH - entryRect.w) / 2;
            SDL_RenderTexture(renderer, entryTexture, NULL, &entryRect);
            SDL_DestroyTexture(entryTexture);
            current = current->next;
            yPos += 50;
            rank++;
        }
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(subtitleTexture);
    return STATE_MENU;
}

