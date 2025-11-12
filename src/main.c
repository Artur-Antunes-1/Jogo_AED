#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <curl/curl.h>

#include <stdio.h>

#include "game.h"
#include "leaderboard.h"
#include "states/leaderboard_state.h"
#include "states/menu_state.h"
#include "states/options_state.h"
#include "states/playing_state.h"
#include "states/scoring_state.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Falha ao inicializar SDL: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        SDL_Log("Falha ao inicializar SDL_ttf: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        SDL_Log("Falha ao inicializar cURL");
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    GameContext context = {0};
    init_default_colors(&context.colors);

    context.window = SDL_CreateWindow("Adedonha (Stop!) - Projeto AED", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!context.window) {
        SDL_Log("Não foi possível criar a janela: %s", SDL_GetError());
        curl_global_cleanup();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    context.renderer = SDL_CreateRenderer(context.window, NULL);
    if (!context.renderer) {
        SDL_Log("Não foi possível criar o renderer: %s", SDL_GetError());
        SDL_DestroyWindow(context.window);
        curl_global_cleanup();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    context.font_title = TTF_OpenFont("font.ttf", 48);
    context.font_body = TTF_OpenFont("font.ttf", 30);
    if (!context.font_title || !context.font_body) {
        SDL_Log("Erro fatal: não foi possível carregar 'font.ttf'.");
        if (context.font_title) {
            TTF_CloseFont(context.font_title);
        }
        if (context.font_body) {
            TTF_CloseFont(context.font_body);
        }
        SDL_DestroyRenderer(context.renderer);
        SDL_DestroyWindow(context.window);
        curl_global_cleanup();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    for (int i = 0; i < 26; i++) {
        context.isLetterEnabled[i] = 1;
    }

    updateScore(&(context.leaderboard), "CPU 1", 50);
    updateScore(&(context.leaderboard), "Jogador", 20);
    updateScore(&(context.leaderboard), "CPU 2", 80);

    GameState currentState = STATE_MENU;
    while (currentState != STATE_EXIT) {
        switch (currentState) {
            case STATE_MENU:
                currentState = runMenu(&context);
                break;
            case STATE_PLAYING:
                currentState = runPlaying(&context);
                break;
            case STATE_SCORING:
                currentState = runScoring(&context);
                break;
            case STATE_LEADERBOARD:
                currentState = runLeaderboard(&context);
                break;
            case STATE_OPTIONS:
                currentState = runOptions(&context);
                break;
            case STATE_EXIT:
            default:
                currentState = STATE_EXIT;
                break;
        }
    }

    freeLeaderboard(&(context.leaderboard));
    TTF_CloseFont(context.font_title);
    TTF_CloseFont(context.font_body);
    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    curl_global_cleanup();
    TTF_Quit();
    return 0;
}
