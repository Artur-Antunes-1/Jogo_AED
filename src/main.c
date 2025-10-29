#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

// --- Constantes do Jogo ---
#define MAX_INPUT_LENGTH 50
#define NUM_THEMES 5

// --- Estrutura para um Campo de Input ---
typedef struct {
    char text[MAX_INPUT_LENGTH];
    SDL_Texture* texture;
    SDL_FRect rect;
    SDL_FRect labelRect;
    SDL_Texture* labelTexture;
} InputField;

// Função auxiliar para renderizar texto
void createTextTexture(SDL_Renderer* renderer, TTF_Font* font, const char* text, 
                       SDL_Texture** texture, SDL_FRect* rect, int x, int y, SDL_Color color) {
    
    if (*texture) {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }

    if (!text || text[0] == '\0') {
        rect->w = 0;
        rect->h = 0;
        return;
    }

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, strlen(text), color);
    if (!textSurface) {
        SDL_Log("Erro ao criar superficie de texto: %s", SDL_GetError());
        return;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    
    rect->x = (float)x;
    rect->y = (float)y;
    rect->w = (float)textSurface->w;
    rect->h = (float)textSurface->h;
    
    SDL_DestroySurface(textSurface);
}


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("Jogo de Stop/Adedonha!", 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    TTF_Font* font = TTF_OpenFont("font.ttf", 24);

    if (!font) {
        SDL_Log("Erro ao carregar fonte 'font.ttf'. Verifique se o arquivo está na pasta raiz.");
        return 1;
    }

    // ----- A LÓGICA DA "IA" -----
    srand((unsigned int)time(NULL));

    const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXZ";
    const char* masterThemeList[] = {
        "Nome", "Animal", "Fruta", "Cor", "Objeto", "CEP", 
        "Carro", "Profissão", "Verbo", "Comida", "Parte do Corpo"
    };
    int letterCount = (int)strlen(letters);
    int themeCount = sizeof(masterThemeList) / sizeof(masterThemeList[0]);

    char chosenLetter = letters[rand() % letterCount];
    const char* chosenThemes[NUM_THEMES];
    int chosenIndexes[NUM_THEMES] = {-1, -1, -1, -1, -1};
    for (int i = 0; i < NUM_THEMES; i++) {
        int newIndex;
        int isRepeated;
        do {
            isRepeated = 0;
            newIndex = rand() % themeCount;
            for (int j = 0; j < i; j++) {
                if (chosenIndexes[j] == newIndex) {
                    isRepeated = 1;
                    break;
                }
            }
        } while (isRepeated);
        chosenThemes[i] = masterThemeList[newIndex];
        chosenIndexes[i] = newIndex;
    }
    // ----- FIM DA LÓGICA DA IA -----


    // ----- PREPARAÇÃO DAS TEXTURAS ESTÁTICAS -----
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {200, 200, 200, 255};

    SDL_Texture* letterTexture = NULL;
    SDL_FRect letterRect;
    char letterText[30];
    sprintf(letterText, "Letra Sorteada: %c", chosenLetter);
    createTextTexture(renderer, font, letterText, &letterTexture, &letterRect, 50, 30, white);

    // ----- PREPARAÇÃO DOS CAMPOS DE INPUT -----
    InputField fields[NUM_THEMES];
    int activeField = 0;

    for (int i = 0; i < NUM_THEMES; i++) {
        strcpy(fields[i].text, "");
        fields[i].texture = NULL;
        fields[i].labelTexture = NULL;

        char label[100];
        sprintf(label, "%s:", chosenThemes[i]);
        createTextTexture(renderer, font, label, 
                          &fields[i].labelTexture, &fields[i].labelRect, 50, 100 + (i * 60), gray);
        
        fields[i].rect.x = 250;
        fields[i].rect.y = 100 + (i * 60);
        fields[i].rect.w = 0;
        fields[i].rect.h = 0;
    }

    SDL_StartTextInput(window);

    // ----- O GAME LOOP -----
    int running = 1;
    SDL_Event event;

    while (running) {
        int textChanged = 0;

        // 1. Eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                // CORREÇÃO AQUI: event.key.key
                if (event.key.key == SDLK_BACKSPACE && strlen(fields[activeField].text) > 0) {
                    fields[activeField].text[strlen(fields[activeField].text) - 1] = '\0';
                    textChanged = 1;
                }
                // CORREÇÃO AQUI: event.key.key
                else if (event.key.key == SDLK_TAB) {
                    activeField = (activeField + 1) % NUM_THEMES;
                }
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (strlen(fields[activeField].text) + strlen(event.text.text) < MAX_INPUT_LENGTH) {
                    strcat(fields[activeField].text, event.text.text);
                    textChanged = 1;
                }
            }
        }

        // 2. Lógica (Update)
        if (textChanged) {
            createTextTexture(renderer, font, fields[activeField].text, 
                              &fields[activeField].texture, &fields[activeField].rect, 
                              250, 100 + (activeField * 60), white);
        }

        // 3. Renderização (Desenho)
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, letterTexture, NULL, &letterRect);
        
        for (int i = 0; i < NUM_THEMES; i++) {
            SDL_RenderTexture(renderer, fields[i].labelTexture, NULL, &fields[i].labelRect);
            SDL_RenderTexture(renderer, fields[i].texture, NULL, &fields[i].rect);

            if (i == activeField) {
                SDL_FRect cursorRect = fields[i].rect;
                if (strlen(fields[i].text) == 0) {
                    cursorRect.x = 250;
                    cursorRect.w = 10;
                    cursorRect.h = 30;
                } else {
                    cursorRect.x = fields[i].rect.x + fields[i].rect.w + 5;
                    cursorRect.w = 10;
                    cursorRect.h = 30;
                }
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &cursorRect);
            }
        }

        SDL_RenderPresent(renderer);
    }
    // ----- Fim do Game Loop -----

    SDL_StopTextInput(window);

    // Limpeza
    SDL_DestroyTexture(letterTexture);
    for (int i = 0; i < NUM_THEMES; i++) {
        SDL_DestroyTexture(fields[i].texture);
        SDL_DestroyTexture(fields[i].labelTexture);
    }
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}