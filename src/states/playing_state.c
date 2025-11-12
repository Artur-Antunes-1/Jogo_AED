#include "playing_state.h"

#include <SDL3/SDL.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ai_service.h"
#include "string_utils.h"
#include "text_utils.h"

typedef struct InputField {
    char text[MAX_INPUT_LENGTH];
    SDL_Texture* texture;
    SDL_FRect rect;
    SDL_FRect labelRect;
    SDL_Texture* labelTexture;
    SDL_FRect inputBoxRect;
} InputField;

typedef struct InputNode {
    InputField field;
    struct InputNode* next;
} InputNode;

static void destroyInputList(InputNode* head) {
    if (!head) {
        return;
    }

    InputNode* current = head;
    InputNode* firstNode = head;
    do {
        InputNode* nextNode = current->next;
        if (current->field.texture) {
            SDL_DestroyTexture(current->field.texture);
        }
        if (current->field.labelTexture) {
            SDL_DestroyTexture(current->field.labelTexture);
        }
        free(current);
        current = nextNode;
    } while (current != firstNode);
}

GameState runPlaying(GameContext* context) {
    if (!context) {
        return STATE_EXIT;
    }

    SDL_Renderer* renderer = context->renderer;

    const Uint64 ROUND_DURATION_MS = 60000;
    Uint64 startTime;
    SDL_Texture* timerTexture = NULL;
    SDL_FRect timerRect;
    char timerText[20];
    int lastSecond = 61;

    SDL_Color white = context->colors.textColor;
    SDL_Color gray = context->colors.accentGray;
    SDL_Color red = context->colors.accentRed;
    SDL_Color inputBg = context->colors.inputBgColor;

    srand((unsigned int)time(NULL));
    char letterPool[27];
    int poolCount = 0;
    for (int i = 0; i < 26; i++) {
        if (context->isLetterEnabled[i]) {
            letterPool[poolCount] = 'A' + i;
            poolCount++;
        }
    }
    letterPool[poolCount] = '\0';

    if (poolCount == 0) {
        SDL_Texture* errTexture = NULL;
        SDL_FRect errRect;
        createTextTexture(context, 1, "Erro: Nenhuma letra ativada!", &errTexture, &errRect, 0, 300, red);
        errRect.x = (SCREEN_WIDTH - errRect.w) / 2;

        SDL_Texture* helpTexture = NULL;
        SDL_FRect helpRect;
        createTextTexture(context, 0, "Vá em 'Opções' para ativar.", &helpTexture, &helpRect, 0, 360, white);
        helpRect.x = (SCREEN_WIDTH - helpRect.w) / 2;

        SDL_SetRenderDrawColor(renderer, context->colors.bgColor.r, context->colors.bgColor.g, context->colors.bgColor.b, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, errTexture, NULL, &errRect);
        SDL_RenderTexture(renderer, helpTexture, NULL, &helpRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(3000);
        SDL_DestroyTexture(errTexture);
        SDL_DestroyTexture(helpTexture);
        return STATE_MENU;
    }

    char chosenLetter = letterPool[rand() % poolCount];

    char prompt[1024];
    sprintf(prompt,
            "Você é um criador de jogos de 'Stop!' (Adedonha) criativo e desafiador. "
            "Sua tarefa é gerar %d temas para a letra '%c'. "
            "Os temas devem ser uma mistura de categorias comuns e algumas categorias mais incomuns ou específicas. "
            "Evite temas EXTREMAMENTE genéricos como 'Cor' ou 'Fruta'. "
            "Prefira temas como 'Personagem de ficção', 'País da Europa', 'Algo que se compra no supermercado', 'Marca de carro', 'Profissão'. "
            "REGRA CRÍTICA: Para CADA tema, você DEVE garantir que exista pelo menos uma resposta razoavelmente comum em português que comece com a letra '%c'. "
            "Não crie temas impossíveis (ex: 'Oceano' para a letra 'W'). "
            "Responda APENAS com os %d temas, separados por vírgula, sem espaços extras após a vírgula e sem quebra de linha. "
            "Exemplo de resposta: País,Marca de roupa,Profissão,Vilão de filme,Coisa que flutua",
            NUM_THEMES, chosenLetter, chosenLetter, NUM_THEMES);

    SDL_Texture* loadingTexture = NULL;
    SDL_FRect loadingRect;
    createTextTexture(context, 1, "Sorteando temas com a IA...", &loadingTexture, &loadingRect, 0, 300, white);
    loadingRect.x = (SCREEN_WIDTH - loadingRect.w) / 2;
    SDL_SetRenderDrawColor(renderer, context->colors.bgColor.r, context->colors.bgColor.g, context->colors.bgColor.b, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, loadingTexture, NULL, &loadingRect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(loadingTexture);

    char* ai_response = call_gemini_api(prompt);
    if (ai_response == NULL) {
        SDL_Texture* errTexture = NULL;
        SDL_FRect errRect;
        createTextTexture(context, 1, "Erro: Falha ao contatar a IA.", &errTexture, &errRect, 0, 300, red);
        errRect.x = (SCREEN_WIDTH - errRect.w) / 2;

        SDL_Texture* helpTexture = NULL;
        SDL_FRect helpRect;
        createTextTexture(context, 0, "Verifique sua API Key ou conexão.", &helpTexture, &helpRect, 0, 360, white);
        helpRect.x = (SCREEN_WIDTH - helpRect.w) / 2;

        SDL_SetRenderDrawColor(renderer, context->colors.bgColor.r, context->colors.bgColor.g, context->colors.bgColor.b, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, errTexture, NULL, &errRect);
        SDL_RenderTexture(renderer, helpTexture, NULL, &helpRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(3000);
        SDL_DestroyTexture(errTexture);
        SDL_DestroyTexture(helpTexture);
        return STATE_MENU;
    }

    const char* chosenThemes[NUM_THEMES];
    int themeCount = 0;
    char* token = strtok(ai_response, ",");
    while (token != NULL && themeCount < NUM_THEMES) {
        while (*token == ' ') {
            token++;
        }
        chosenThemes[themeCount] = token;
        themeCount++;
        token = strtok(NULL, ",");
    }
    for (int i = themeCount; i < NUM_THEMES; i++) {
        chosenThemes[i] = "Erro da IA";
    }

    quickSortStrings(chosenThemes, 0, NUM_THEMES - 1);

    SDL_Texture* letterTexture = NULL;
    SDL_FRect letterRect;
    char letterText[30];
    sprintf(letterText, "Letra Sorteada: %c", chosenLetter);
    int topRowY = 50;
    createTextTexture(context, 1, letterText, &letterTexture, &letterRect, 50, topRowY, white);

    InputNode* headInput = NULL;
    InputNode* currentInput = NULL;
    InputNode* prevInput = NULL;

    float inputYStart = 150.0f;
    float inputHeight = 60.0f;
    float inputSpacing = 80.0f;
    float labelX = 100.0f;
    float inputX = 600.0f;
    float inputWidth = 850.0f;
    float textPaddingY = (inputHeight - TTF_GetFontHeight(context->font_body)) / 2.0f;

    for (int i = 0; i < NUM_THEMES; i++) {
        currentInput = (InputNode*)malloc(sizeof(InputNode));
        if (!currentInput) {
            free(ai_response);
            destroyInputList(headInput);
            SDL_DestroyTexture(letterTexture);
            SDL_DestroyTexture(timerTexture);
            return STATE_EXIT;
        }
        currentInput->next = NULL;

        InputField* field = &(currentInput->field);
        field->text[0] = '\0';
        field->texture = NULL;
        field->labelTexture = NULL;

        char label[100];
        sprintf(label, "%s:", chosenThemes[i]);
        createTextTexture(context, 0, label,
                          &(field->labelTexture), &(field->labelRect),
                          (int)labelX, (int)(inputYStart + (i * inputSpacing) + textPaddingY), gray);

        field->inputBoxRect.x = inputX;
        field->inputBoxRect.y = inputYStart + (i * inputSpacing);
        field->inputBoxRect.w = inputWidth;
        field->inputBoxRect.h = inputHeight;
        field->rect.x = inputX + 10;
        field->rect.y = inputYStart + (i * inputSpacing) + textPaddingY;
        field->rect.w = 0;
        field->rect.h = 0;

        if (headInput == NULL) {
            headInput = currentInput;
        } else {
            prevInput->next = currentInput;
        }
        prevInput = currentInput;
    }
    if (prevInput != NULL) {
        prevInput->next = headInput;
    }

    InputNode* activeNode = headInput;
    SDL_StartTextInput(context->window);

    startTime = SDL_GetTicks();
    int running_playing = 1;
    SDL_Event event;
    GameState nextState = STATE_MENU;

    if (activeNode == NULL) {
        running_playing = 0;
    }

    while (running_playing) {
        int textChanged = 0;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                nextState = STATE_EXIT;
                running_playing = 0;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    nextState = STATE_MENU;
                    running_playing = 0;
                } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                    nextState = STATE_SCORING;
                    running_playing = 0;
                } else if (event.key.key == SDLK_BACKSPACE && strlen(activeNode->field.text) > 0) {
                    activeNode->field.text[strlen(activeNode->field.text) - 1] = '\0';
                    textChanged = 1;
                } else if (event.key.key == SDLK_TAB) {
                    activeNode = activeNode->next;
                }
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (strlen(activeNode->field.text) + strlen(event.text.text) < MAX_INPUT_LENGTH) {
                    strcat(activeNode->field.text, event.text.text);
                    textChanged = 1;
                }
            }
        }

        Uint64 elapsedMs = SDL_GetTicks() - startTime;
        int secondsLeft = (int)((ROUND_DURATION_MS - elapsedMs) / 1000);

        if (elapsedMs >= ROUND_DURATION_MS) {
            secondsLeft = 0;
            nextState = STATE_SCORING;
            running_playing = 0;
        }
        if (secondsLeft < 0) {
            secondsLeft = 0;
        }

        if (secondsLeft != lastSecond) {
            sprintf(timerText, "Tempo: %d", secondsLeft);
            createTextTexture(context, 1, timerText, &timerTexture, &timerRect, 0, topRowY, white);
            timerRect.x = SCREEN_WIDTH - timerRect.w - 50;
            lastSecond = secondsLeft;
        }

        if (textChanged) {
            createTextTexture(context, 0, activeNode->field.text,
                              &(activeNode->field.texture), &(activeNode->field.rect),
                              (int)(activeNode->field.inputBoxRect.x + 10),
                              (int)(activeNode->field.inputBoxRect.y + textPaddingY), white);
        }

        SDL_SetRenderDrawColor(renderer, context->colors.bgColor.r, context->colors.bgColor.g, context->colors.bgColor.b, 255);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, letterTexture, NULL, &letterRect);
        SDL_RenderTexture(renderer, timerTexture, NULL, &timerRect);

        InputNode* tempNode = headInput;
        do {
            InputField* field = &(tempNode->field);
            SDL_SetRenderDrawColor(renderer, inputBg.r, inputBg.g, inputBg.b, inputBg.a);
            SDL_RenderFillRect(renderer, &(field->inputBoxRect));
            SDL_RenderTexture(renderer, field->labelTexture, NULL, &(field->labelRect));
            SDL_RenderTexture(renderer, field->texture, NULL, &(field->rect));

            if (tempNode == activeNode) {
                SDL_FRect cursorRect;
                if (strlen(field->text) == 0) {
                    if ((SDL_GetTicks() / 500) % 2 == 0) {
                        cursorRect.x = field->inputBoxRect.x + 10;
                        cursorRect.y = field->inputBoxRect.y + (field->inputBoxRect.h - 30) / 2;
                        cursorRect.w = 10;
                        cursorRect.h = 30;
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_RenderFillRect(renderer, &cursorRect);
                    }
                } else {
                    if ((SDL_GetTicks() / 500) % 2 == 0) {
                        cursorRect.x = field->rect.x + field->rect.w + 5;
                        cursorRect.y = field->inputBoxRect.y + (field->inputBoxRect.h - 30) / 2;
                        cursorRect.w = 10;
                        cursorRect.h = 30;
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_RenderFillRect(renderer, &cursorRect);
                    }
                }
                SDL_SetRenderDrawColor(renderer, context->colors.titleColor.r, context->colors.titleColor.g, context->colors.titleColor.b, 255);
                SDL_RenderRect(renderer, &(field->inputBoxRect));
            }
            tempNode = tempNode->next;
        } while (tempNode != headInput);

        SDL_RenderPresent(renderer);
    }

    if (nextState == STATE_SCORING) {
        context->lastLetter = chosenLetter;
        InputNode* tempNode = headInput;
        int i = 0;
        do {
            strcpy(context->lastThemes[i], chosenThemes[i]);
            strcpy(context->lastAnswers[i], tempNode->field.text);
            tempNode = tempNode->next;
            i++;
        } while (tempNode != headInput && i < NUM_THEMES);
    }

    SDL_StopTextInput(context->window);
    SDL_DestroyTexture(letterTexture);
    SDL_DestroyTexture(timerTexture);
    destroyInputList(headInput);
    free(ai_response);

    return nextState;
}

