#include "scoring_state.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_service.h"
#include "leaderboard.h"
#include "text_utils.h"

static void trimTrailingWhitespace(char* str) {
    if (!str) {
        return;
    }
    size_t len = strlen(str);
    while (len > 0) {
        char c = str[len - 1];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            str[len - 1] = '\0';
            len--;
        } else {
            break;
        }
    }
}

GameState runScoring(GameContext* context) {
    if (!context) {
        return STATE_EXIT;
    }

    SDL_Renderer* renderer = context->renderer;
    SDL_Color white = context->colors.textColor;
    SDL_Color gray = context->colors.accentGray;
    SDL_Color green = context->colors.accentGreen;
    SDL_Color red = context->colors.accentRed;

    SDL_Texture* loadingTexture = NULL;
    SDL_FRect loadingRect;
    createTextTexture(context, 1, "IA está julgando suas respostas...", &loadingTexture, &loadingRect, 0, 300, white);
    loadingRect.x = (SCREEN_WIDTH - loadingRect.w) / 2;
    SDL_SetRenderDrawColor(renderer, context->colors.bgColor.r, context->colors.bgColor.g, context->colors.bgColor.b, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, loadingTexture, NULL, &loadingRect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(loadingTexture);

    int scoreThisRound = 0;
    int scores[NUM_THEMES] = {0};

    char validation_prompt[2048];
    char temp_prompt[512];
    sprintf(validation_prompt,
            "Você é um juiz do jogo Adedonha (Stop!) para a letra '%c'. "
            "Valide a seguinte lista de tema-resposta. "
            "Para cada item, responda APENAS 'Sim' se a resposta for válida e começar com a letra '%c', ou 'Nao' caso contrário. "
            "Responda apenas com 'Sim' ou 'Nao' para cada item, separados por vírgula. "
            "Não adicione nenhuma outra palavra. "
            "Exemplo de Resposta: Sim,Nao,Sim,Sim,Nao\n\n"
            "A validar:\n", context->lastLetter, context->lastLetter);

    for (int i = 0; i < NUM_THEMES; i++) {
        if (strlen(context->lastAnswers[i]) == 0) {
            sprintf(temp_prompt, "Tema: '%s', Resposta: ''\n", context->lastThemes[i]);
        } else {
            sprintf(temp_prompt, "Tema: '%s', Resposta: '%s'\n", context->lastThemes[i], context->lastAnswers[i]);
        }
        strcat(validation_prompt, temp_prompt);
    }

    char* ai_response = call_gemini_api(validation_prompt);
    if (ai_response) {
        SDL_Log("IA (validação) respondeu: %s", ai_response);
        char* token = strtok(ai_response, ",");
        int i = 0;
        while (token != NULL && i < NUM_THEMES) {
            while (*token == ' ' || *token == '\n') {
                token++;
            }
            trimTrailingWhitespace(token);
            SDL_Log("Tema %d: resposta '%s' => AI '%s'", i, context->lastAnswers[i], token);

            if (SDL_strncasecmp(token, "Sim", 3) == 0) {
                scores[i] = 10;
                scoreThisRound += 10;
            } else {
                scores[i] = 0;
            }

            token = strtok(NULL, ",");
            i++;
        }
        free(ai_response);
    } else {
        scoreThisRound = 0;
        for (int i = 0; i < NUM_THEMES; i++) {
            scores[i] = 0;
        }
    }

    updateScore(&(context->leaderboard), "Jogador", scoreThisRound);

    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    char scoreText[100];
    sprintf(scoreText, "Você fez %d pontos nesta rodada! (Letra %c)", scoreThisRound, context->lastLetter);
    createTextTexture(context, 1, scoreText, &titleTexture, &titleRect, 0, 100, white);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;

    SDL_Texture* answerTextures[NUM_THEMES];
    SDL_FRect answerRects[NUM_THEMES];
    for (int i = 0; i < NUM_THEMES; i++) {
        char answerLine[200];
        sprintf(answerLine, "%s: %s [%d pts]", context->lastThemes[i],
                (strlen(context->lastAnswers[i]) > 0) ? context->lastAnswers[i] : "-",
                scores[i]);

        answerTextures[i] = NULL;
        SDL_Color aColor = (scores[i] > 0) ? green : ((strlen(context->lastAnswers[i]) > 0) ? red : gray);

        createTextTexture(context, 0, answerLine,
                          &answerTextures[i], &answerRects[i], 200, 200 + (i * 50), aColor);
    }

    SDL_Texture* subtitleTexture = NULL;
    SDL_FRect subtitleRect;
    createTextTexture(context, 0, "Pressione ESC para voltar ao Menu", &subtitleTexture, &subtitleRect, 0, 650, white);
    subtitleRect.x = (SCREEN_WIDTH - subtitleRect.w) / 2;

    int running_scoring = 1;
    SDL_Event event;
    while (running_scoring) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running_scoring = 0;
                SDL_DestroyTexture(titleTexture);
                SDL_DestroyTexture(subtitleTexture);
                for (int i = 0; i < NUM_THEMES; i++) {
                    SDL_DestroyTexture(answerTextures[i]);
                }
                return STATE_EXIT;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running_scoring = 0;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, context->colors.bgColor.r, context->colors.bgColor.g, context->colors.bgColor.b, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderTexture(renderer, subtitleTexture, NULL, &subtitleRect);
        for (int i = 0; i < NUM_THEMES; i++) {
            SDL_RenderTexture(renderer, answerTextures[i], NULL, &answerRects[i]);
        }
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(subtitleTexture);
    for (int i = 0; i < NUM_THEMES; i++) {
        SDL_DestroyTexture(answerTextures[i]);
    }

    return STATE_MENU;
}

