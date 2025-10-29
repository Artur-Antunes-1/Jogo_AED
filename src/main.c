#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

// --- NOVO: Constantes de Tela ---
#define MAX_INPUT_LENGTH 50
#define NUM_THEMES 5
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// --- REQ. 2: Estados do Jogo ---
typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_SCORING,
    STATE_LEADERBOARD,
    STATE_OPTIONS,
    STATE_EXIT
} GameState;

// --- Estrutura para os Inputs ---
typedef struct {
    char text[MAX_INPUT_LENGTH];
    SDL_Texture* texture;
    SDL_FRect rect;
    SDL_FRect labelRect;
    SDL_Texture* labelTexture;
    SDL_FRect inputBoxRect; // Guarda o retângulo da caixa cinza
} InputField;

// --- NOVO: REQ. 3 (Adicional): Lista Circular para Inputs ---
typedef struct InputNode {
    InputField field;         // Contém os dados do campo (texto, texturas, etc.)
    struct InputNode* next; // Ponteiro para o próximo nó na lista circular
} InputNode;

// --- REQ. 3: Estrutura da Lista Dupla ---
typedef struct PlayerNode {
    char name[50];
    int totalScore;
    struct PlayerNode* next;
    struct PlayerNode* prev;
} PlayerNode;

// --- Estrutura de "Contexto" ---
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_title; // Fonte grande
    TTF_Font* font_body;  // Fonte normal
    PlayerNode* leaderboard;
    char lastLetter;
    char lastThemes[NUM_THEMES][100];
    char lastAnswers[NUM_THEMES][MAX_INPUT_LENGTH];
    int isLetterEnabled[26];
} GameContext;

// --- Função Auxiliar de Texto (Atualizada) ---
void createTextTexture(GameContext* context, int isTitleFont, const char* text, 
                       SDL_Texture** texture, SDL_FRect* rect, int x, int y, SDL_Color color) {
    
    if (*texture) {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }
    if (!text || text[0] == '\0') {
        rect->w = 0; rect->h = 0;
        return;
    }
    
    TTF_Font* fontToUse = (isTitleFont) ? context->font_title : context->font_body;
    SDL_Surface* textSurface = TTF_RenderText_Blended(fontToUse, text, strlen(text), color);
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

// --- Funções da Estrutura de Dados (REQ. 5) ---
PlayerNode* createPlayer(const char* name) {
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (newNode) {
        strcpy(newNode->name, name);
        newNode->totalScore = 0;
        newNode->next = NULL;
        newNode->prev = NULL;
    }
    return newNode;
}
void insertSorted(PlayerNode** head, PlayerNode* newNode) {
    if (*head == NULL || (*head)->totalScore <= newNode->totalScore) {
        newNode->next = *head;
        if (*head != NULL) { (*head)->prev = newNode; }
        *head = newNode;
    } else {
        PlayerNode* current = *head;
        while (current->next != NULL && current->next->totalScore > newNode->totalScore) {
            current = current->next;
        }
        newNode->next = current->next;
        if (current->next != NULL) { current->next->prev = newNode; }
        current->next = newNode;
        newNode->prev = current;
    }
}
PlayerNode* findByName(PlayerNode* head, const char* name) {
    PlayerNode* current = head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) { return current; }
        current = current->next;
    }
    return NULL;
}
void removePlayer(PlayerNode** head, PlayerNode* playerToRemove) {
    if (*head == NULL || playerToRemove == NULL) return;
    if (*head == playerToRemove) { *head = playerToRemove->next; }
    if (playerToRemove->prev != NULL) { playerToRemove->prev->next = playerToRemove->next; }
    if (playerToRemove->next != NULL) { playerToRemove->next->prev = playerToRemove->prev; }
    playerToRemove->prev = NULL;
    playerToRemove->next = NULL;
}
void freeList(PlayerNode** head) {
    PlayerNode* current = *head;
    PlayerNode* nextNode;
    while (current != NULL) {
        nextNode = current->next;
        free(current);
        current = nextNode;
    }
    *head = NULL;
}
void updateScore(PlayerNode** head, const char* name, int pointsToAdd) {
    PlayerNode* player = findByName(*head, name);
    if (player == NULL) { player = createPlayer(name); }
    else { removePlayer(head, player); }
    player->totalScore += pointsToAdd;
    insertSorted(head, player);
}

// --- Algoritmo de Ordenação (REQ. 4) ---
void swapStrings(const char** a, const char** b) { const char* t = *a; *a = *b; *b = t; }
int partition(const char* arr[], int low, int high) {
    const char* pivot = arr[high]; int i = (low - 1);
    for (int j = low; j <= high - 1; j++) {
        if (strcmp(arr[j], pivot) <= 0) {
            i++; swapStrings(&arr[i], &arr[j]);
        }
    }
    swapStrings(&arr[i + 1], &arr[high]);
    return (i + 1);
}
void quickSortStrings(const char* arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSortStrings(arr, low, pi - 1);
        quickSortStrings(arr, pi + 1, high);
    }
}


// --- LÓGICA DO ESTADO: JOGANDO (COM LISTA CIRCULAR) ---
GameState runPlaying(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;

    const Uint64 ROUND_DURATION_MS = 60000;
    Uint64 startTime = SDL_GetTicks(); 

    SDL_Texture* timerTexture = NULL;
    SDL_FRect timerRect;
    char timerText[20];
    int lastSecond = -1;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {200, 200, 200, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color inputBg = {50, 50, 50, 255};

    // ----- Sorteio de Letra -----
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

    char chosenLetter;
    if (poolCount == 0) { 
        // Mostra erro (agora centralizado)
        SDL_Texture* errTexture = NULL;
        SDL_FRect errRect;
        createTextTexture(context, 1, "Erro: Nenhuma letra ativada!", &errTexture, &errRect, 0, 300, red);
        errRect.x = (SCREEN_WIDTH - errRect.w) / 2;
        
        SDL_Texture* helpTexture = NULL;
        SDL_FRect helpRect;
        createTextTexture(context, 0, "Vá em 'Opções' para ativar.", &helpTexture, &helpRect, 0, 360, white);
        helpRect.x = (SCREEN_WIDTH - helpRect.w) / 2;
        
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, errTexture, NULL, &errRect);
        SDL_RenderTexture(renderer, helpTexture, NULL, &helpRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(3000);
        SDL_DestroyTexture(errTexture);
        SDL_DestroyTexture(helpTexture);
        return STATE_MENU;
    }
    chosenLetter = letterPool[rand() % poolCount];

    // ----- Sorteio de Temas -----
    const char* masterThemeList[] = {
        "Nome", "Animal", "Fruta", "Cor", "Objeto", "CEP", 
        "Carro", "Profissão", "Verbo", "Comida", "Parte do Corpo"
    };
    int themeCount = sizeof(masterThemeList) / sizeof(masterThemeList[0]);
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
    quickSortStrings(chosenThemes, 0, NUM_THEMES - 1);

    // ----- Preparação das Texturas e Inputs (AGORA COM LISTA CIRCULAR) -----
    SDL_Texture* letterTexture = NULL;
    SDL_FRect letterRect;
    char letterText[30];
    sprintf(letterText, "Letra Sorteada: %c", chosenLetter);
    int topRowY = 50;
    createTextTexture(context, 1, letterText, &letterTexture, &letterRect, 50, topRowY, white);

    // --- Criação da Lista Circular de Inputs ---
    InputNode* headInput = NULL;
    InputNode* currentInput = NULL;
    InputNode* prevInput = NULL;

    float inputYStart = 150.0f;
    float inputHeight = 50.0f;
    float inputSpacing = 70.0f;
    float labelX = 100.0f;
    float inputX = 350.0f;
    float inputWidth = 700.0f;
    float textPaddingY = 10.0f;

    for (int i = 0; i < NUM_THEMES; i++) {
        // Aloca o novo nó
        currentInput = (InputNode*)malloc(sizeof(InputNode));
        if (!currentInput) { 
            SDL_Log("Erro: Falha ao alocar memória para InputNode %d", i);
            // Limpeza parcial (liberar nós já criados) - Implementação omitida por brevidade
            return STATE_EXIT; 
        }
        currentInput->next = NULL; // Importante inicializar

        // Inicializa o campo dentro do nó
        InputField* field = &(currentInput->field);
        strcpy(field->text, "");
        field->texture = NULL;
        field->labelTexture = NULL;

        // Cria a Label
        char label[100];
        sprintf(label, "%s:", chosenThemes[i]);
        createTextTexture(context, 0, label, 
                          &(field->labelTexture), &(field->labelRect), 
                          (int)labelX, (int)(inputYStart + (i * inputSpacing) + textPaddingY), gray);
        
        // Define a posição da caixa cinza
        field->inputBoxRect.x = inputX;
        field->inputBoxRect.y = inputYStart + (i * inputSpacing);
        field->inputBoxRect.w = inputWidth;
        field->inputBoxRect.h = inputHeight;
        
        // Define a posição inicial do texto (dentro da caixa)
        field->rect.x = inputX + 10;
        field->rect.y = inputYStart + (i * inputSpacing) + textPaddingY;
        field->rect.w = 0;
        field->rect.h = 0;

        // Conecta o nó na lista
        if (headInput == NULL) {
            headInput = currentInput;
        } else {
            prevInput->next = currentInput;
        }
        prevInput = currentInput;
    }
    // Fecha o círculo
    if (prevInput != NULL) {
        prevInput->next = headInput;
    }
    // --- FIM DA CRIAÇÃO DA LISTA ---

    InputNode* activeNode = headInput; // O nó ativo começa no primeiro

    SDL_StartTextInput(context->window);

    // ----- Game Loop (Playing) -----
    int running_playing = 1;
    SDL_Event event;
    GameState nextState = STATE_MENU; 

    // Garante que activeNode não é NULL antes de entrar no loop (caso NUM_THEMES seja 0)
    if (activeNode == NULL) {
        SDL_Log("Erro: Lista de Inputs vazia.");
        return STATE_MENU; // Ou STATE_EXIT
    }

    while (running_playing) {
        int textChanged = 0;
        // 1. Eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                nextState = STATE_EXIT; running_playing = 0;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    nextState = STATE_MENU; running_playing = 0;
                }
                else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                    nextState = STATE_SCORING; running_playing = 0;
                }
                else if (event.key.key == SDLK_BACKSPACE && strlen(activeNode->field.text) > 0) {
                    // Modifica o texto no NÓ ATIVO
                    activeNode->field.text[strlen(activeNode->field.text) - 1] = '\0';
                    textChanged = 1;
                }
                else if (event.key.key == SDLK_TAB) {
                    // Navegação com a Lista Circular
                    activeNode = activeNode->next;
                }
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                 // Modifica o texto no NÓ ATIVO
                if (strlen(activeNode->field.text) + strlen(event.text.text) < MAX_INPUT_LENGTH) {
                    strcat(activeNode->field.text, event.text.text);
                    textChanged = 1;
                }
            }
        }

        // 2. Lógica (Update)
        Uint64 elapsedMs = SDL_GetTicks() - startTime;
        int secondsLeft = (int)((ROUND_DURATION_MS - elapsedMs) / 1000);

        if (elapsedMs >= ROUND_DURATION_MS) {
            secondsLeft = 0;
            nextState = STATE_SCORING;
            running_playing = 0;
        }

        if (secondsLeft != lastSecond) {
            sprintf(timerText, "Tempo: %d", secondsLeft);
            createTextTexture(context, 1, timerText, &timerTexture, &timerRect, 0, topRowY, white);
            timerRect.x = SCREEN_WIDTH - timerRect.w - 50;
            lastSecond = secondsLeft;
        }
        
        if (textChanged) {
            // Recria a textura do NÓ ATIVO
            createTextTexture(context, 0, activeNode->field.text, 
                              &(activeNode->field.texture), &(activeNode->field.rect), 
                              (int)(activeNode->field.inputBoxRect.x + 10), 
                              (int)(activeNode->field.inputBoxRect.y + textPaddingY), white);
        }

        // 3. Renderização
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, letterTexture, NULL, &letterRect);
        SDL_RenderTexture(renderer, timerTexture, NULL, &timerRect); 
        
        // --- Percorre a Lista Circular para Desenhar ---
        InputNode* tempNode = headInput;
        do {
            InputField* field = &(tempNode->field); // Ponteiro para o campo atual

            // Desenha a "Caixa de Texto"
            SDL_SetRenderDrawColor(renderer, inputBg.r, inputBg.g, inputBg.b, inputBg.a);
            SDL_RenderFillRect(renderer, &(field->inputBoxRect));
            
            // Desenha a etiqueta (Label) e o Texto
            SDL_RenderTexture(renderer, field->labelTexture, NULL, &(field->labelRect));
            SDL_RenderTexture(renderer, field->texture, NULL, &(field->rect));

            // Desenha o cursor SE for o nó ativo
            if (tempNode == activeNode) {
                SDL_FRect cursorRect;
                if (strlen(field->text) == 0) {
                    cursorRect.x = field->inputBoxRect.x + 10;
                } else {
                    cursorRect.x = field->rect.x + field->rect.w + 5;
                }
                cursorRect.y = field->inputBoxRect.y + (field->inputBoxRect.h - 30) / 2; // Centraliza cursor
                cursorRect.w = 10;
                cursorRect.h = 30;
                
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &cursorRect);
            }
            tempNode = tempNode->next; // Vai para o próximo nó
        } while (tempNode != headInput); // Continua até voltar ao início
        // --- FIM DO DESENHO DA LISTA ---
        
        SDL_RenderPresent(renderer);
    }

    // --- Salva os resultados LENDO a Lista Circular ---
    if (nextState == STATE_SCORING) {
        context->lastLetter = chosenLetter;
        InputNode* tempNode = headInput;
        int i = 0;
        do {
             strcpy(context->lastThemes[i], chosenThemes[i]); // Pega o tema na ordem (já ordenado)
             strcpy(context->lastAnswers[i], tempNode->field.text); // Pega a resposta do nó
             tempNode = tempNode->next;
             i++;
        } while(tempNode != headInput && i < NUM_THEMES);
    }
    
    // --- Limpeza da Lista Circular ---
    SDL_StopTextInput(context->window);
    SDL_DestroyTexture(letterTexture);
    SDL_DestroyTexture(timerTexture); 
    
    InputNode* currentNode = headInput;
    if (currentNode) { // Verifica se a lista não está vazia
        InputNode* nextNode;
        currentNode = headInput; // Garante começar do head
        do {
            nextNode = currentNode->next;
            // Libera as texturas DENTRO do nó
            if (currentNode->field.texture) SDL_DestroyTexture(currentNode->field.texture);
            if (currentNode->field.labelTexture) SDL_DestroyTexture(currentNode->field.labelTexture);
            // Libera o nó em si
            free(currentNode);
            currentNode = nextNode;
        } while (currentNode != headInput); // Continua até voltar ao início (ou até ter liberado todos)
    }
    // --- FIM DA LIMPEZA DA LISTA ---

    return nextState; 
}

// --- LÓGICA DO ESTADO: PONTUAÇÃO (LAYOUT 1280x720) ---
GameState runScoring(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {200, 200, 200, 255};

    int scoreThisRound = 0;
    for (int i = 0; i < NUM_THEMES; i++) {
        if (strlen(context->lastAnswers[i]) > 0) {
            scoreThisRound += 5;
        }
    }
    updateScore(&(context->leaderboard), "Jogador", scoreThisRound);

    // Prepara texturas
    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    char scoreText[100];
    sprintf(scoreText, "Você fez %d pontos nesta rodada! (Letra %c)", scoreThisRound, context->lastLetter);
    createTextTexture(context, 1, scoreText, &titleTexture, &titleRect, 0, 100, white);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2; // Centraliza
    
    SDL_Texture* answerTextures[NUM_THEMES];
    SDL_FRect answerRects[NUM_THEMES];
    for (int i = 0; i < NUM_THEMES; i++) {
        char answerLine[200];
        sprintf(answerLine, "%s: %s", context->lastThemes[i], context->lastAnswers[i]);
        answerTextures[i] = NULL;
        createTextTexture(context, 0, answerLine, 
                          &answerTextures[i], &answerRects[i], 200, 200 + (i * 50), gray); // Posição Y e espaçamento maiores
    }

    SDL_Texture* subtitleTexture = NULL;
    SDL_FRect subtitleRect;
    createTextTexture(context, 0, "Pressione ESC para voltar ao Menu", &subtitleTexture, &subtitleRect, 0, 650, white);
    subtitleRect.x = (SCREEN_WIDTH - subtitleRect.w) / 2; // Centraliza

    // Loop do Estado
    int running_scoring = 1;
    SDL_Event event;
    while(running_scoring) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { return STATE_EXIT; }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) { running_scoring = 0; }
            }
        }
        // Renderização
        SDL_SetRenderDrawColor(renderer, 50, 50, 80, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderTexture(renderer, subtitleTexture, NULL, &subtitleRect);
        for (int i = 0; i < NUM_THEMES; i++) {
            SDL_RenderTexture(renderer, answerTextures[i], NULL, &answerRects[i]);
        }
        SDL_RenderPresent(renderer);
    }
    // Limpeza
    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(subtitleTexture);
    for (int i = 0; i < NUM_THEMES; i++) {
        SDL_DestroyTexture(answerTextures[i]);
    }
    return STATE_MENU;
}


// --- LÓGICA DO ESTADO: PLACAR (LAYOUT 1280x720) ---
GameState runLeaderboard(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

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
    while(running_leaderboard) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { return STATE_EXIT; }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) { running_leaderboard = 0; }
            }
        }
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderTexture(renderer, subtitleTexture, NULL, &subtitleRect);

        PlayerNode* current = context->leaderboard;
        int yPos = 250; // Começa mais baixo
        int rank = 1;
        while(current != NULL && rank <= 5) {
            char scoreEntry[100];
            sprintf(scoreEntry, "%d. %s - %d Pontos", rank, current->name, current->totalScore);
            SDL_Texture* entryTexture = NULL;
            SDL_FRect entryRect;
            createTextTexture(context, 0, scoreEntry, &entryTexture, &entryRect, 0, yPos, white);
            entryRect.x = (SCREEN_WIDTH - entryRect.w) / 2;
            SDL_RenderTexture(renderer, entryTexture, NULL, &entryRect);
            SDL_DestroyTexture(entryTexture);
            current = current->next;
            yPos += 50; // Mais espaçamento
            rank++;
        }
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(subtitleTexture);
    return STATE_MENU;
}


// --- ESTADO: MENU DE OPÇÕES (LAYOUT 1280x720) ---
GameState runOptions(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;

    SDL_Color bgColor = {30, 30, 30, 255};
    SDL_Color titleColor = {255, 255, 0, 255};
    SDL_Color enabledColor = {50, 200, 50, 255};
    SDL_Color disabledColor = {200, 50, 50, 255};
    SDL_Color selectedColor = {255, 255, 255, 255};
    SDL_Color textColor = {200, 200, 200, 255};

    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    createTextTexture(context, 1, "Opções de Letras", &titleTexture, &titleRect, 0, 50, titleColor);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;

    SDL_Texture* helpTexture = NULL;
    SDL_FRect helpRect;
    createTextTexture(context, 0, "Use Setas para Mover, ENTER para Ativar/Desativar, ESC para Voltar", &helpTexture, &helpRect, 0, 650, textColor);
    helpRect.x = (SCREEN_WIDTH - helpRect.w) / 2;

    int selectedLetter = 0;
    
    // Layout da Grade (6 colunas)
    const int numCols = 6;
    const int spacingX = 150; // Mais espaço horizontal
    const int spacingY = 80;
    const int startX = (SCREEN_WIDTH - (spacingX * (numCols - 1))) / 2;
    const int startY = 150;

    int running_options = 1;
    SDL_Event event;

    while (running_options) {
        // ... (Loop de Eventos é o MESMO de antes) ...
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                return STATE_EXIT;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch(event.key.key) {
                    case SDLK_ESCAPE:
                        running_options = 0;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        context->isLetterEnabled[selectedLetter] = !context->isLetterEnabled[selectedLetter];
                        break;
                    // Navegação na grade
                    case SDLK_RIGHT:
                        if ((selectedLetter + 1) % numCols == 0)
                            selectedLetter -= (numCols - 1);
                        else if (selectedLetter + 1 < 26)
                            selectedLetter++;
                        break;
                    case SDLK_LEFT:
                        if (selectedLetter % numCols == 0)
                            selectedLetter += (numCols - 1);
                        else
                            selectedLetter--;
                        if (selectedLetter > 25) selectedLetter = 25; 
                        break;
                    case SDLK_DOWN:
                        if (selectedLetter + numCols < 26)
                            selectedLetter += numCols;
                        break;
                    case SDLK_UP:
                        if (selectedLetter - numCols >= 0)
                            selectedLetter -= numCols;
                        break;
                }
            }
        }

        // Renderização
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderTexture(renderer, helpTexture, NULL, &helpRect);

        // Desenha a grade de 26 letras
        for (int i = 0; i < 26; i++) {
            int row = i / numCols;
            int col = i % numCols;
            int xPos = startX + (col * spacingX);
            int yPos = startY + (row * spacingY);

            // Usa [ON] / [OFF]
            char letterChar = 'A' + i;
            const char* status = context->isLetterEnabled[i] ? "[ON]" : "[OFF]";
            SDL_Color statusColor = context->isLetterEnabled[i] ? enabledColor : disabledColor;
            
            char entryText[20];
            sprintf(entryText, "%c: %s", letterChar, status);

            SDL_Texture* entryTexture = NULL;
            SDL_FRect entryRect;
            createTextTexture(context, 0, entryText, &entryTexture, &entryRect, xPos, yPos, statusColor);
            
            SDL_RenderTexture(renderer, entryTexture, NULL, &entryRect);

            // Desenha o cursor
            if (i == selectedLetter) {
                SDL_FRect cursorRect = {entryRect.x - 10, entryRect.y - 5, 
                                        entryRect.w + 20, entryRect.h + 10};
                SDL_SetRenderDrawColor(renderer, selectedColor.r, selectedColor.g, selectedColor.b, selectedColor.a);
                SDL_RenderRect(renderer, &cursorRect);
            }
            
            SDL_DestroyTexture(entryTexture);
        }
        
        SDL_RenderPresent(renderer);
    }

    // Limpeza
    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(helpTexture);

    return STATE_MENU;
}


// --- LÓGICA DO ESTADO: MENU (LAYOUT 1280x720) ---
GameState runMenu(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;
    
    int selectedOption = 0;
    const int numOptions = 4;
    const int buttonWidth = 400; // Botão mais largo
    const int buttonHeight = 60; // Botão mais alto
    const int buttonYStart = 250;
    const int buttonPadding = 20;

    SDL_Color bgColor = {30, 30, 30, 255};
    SDL_Color buttonColor = {70, 70, 70, 255};
    SDL_Color selectedColor = {100, 100, 20, 255};
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color titleColor = {255, 255, 0, 255};

    // --- Prepara as Texturas de Texto ---
    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    createTextTexture(context, 1, "Jogo de Adedonha (STOP!)", &titleTexture, &titleRect, 0, 100, titleColor);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;

    SDL_Texture* playTexture = NULL; SDL_FRect playRect;
    createTextTexture(context, 0, "Iniciar Jogo", &playTexture, &playRect, 0, 0, textColor);
    
    SDL_Texture* boardTexture = NULL; SDL_FRect boardRect;
    createTextTexture(context, 0, "Ver Placar", &boardTexture, &boardRect, 0, 0, textColor);

    SDL_Texture* optionsTexture = NULL; SDL_FRect optionsRect;
    createTextTexture(context, 0, "Opções de Letras", &optionsTexture, &optionsRect, 0, 0, textColor);

    SDL_Texture* exitTexture = NULL; SDL_FRect exitRect;
    createTextTexture(context, 0, "Sair", &exitTexture, &exitRect, 0, 0, textColor);

    // Retângulos dos Botões
    SDL_FRect playButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart, buttonWidth, buttonHeight };
    SDL_FRect boardButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart + (buttonHeight + buttonPadding), buttonWidth, buttonHeight };
    SDL_FRect optionsButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart + (buttonHeight + buttonPadding) * 2, buttonWidth, buttonHeight };
    SDL_FRect exitButtonRect = { (SCREEN_WIDTH - buttonWidth) / 2, buttonYStart + (buttonHeight + buttonPadding) * 3, buttonWidth, buttonHeight };

    // Centraliza os textos dentro dos seus botões
    playRect.x = (SCREEN_WIDTH - playRect.w) / 2;
    playRect.y = playButtonRect.y + (buttonHeight - playRect.h) / 2;
    boardRect.x = (SCREEN_WIDTH - boardRect.w) / 2;
    boardRect.y = boardButtonRect.y + (buttonHeight - boardRect.h) / 2;
    optionsRect.x = (SCREEN_WIDTH - optionsRect.w) / 2;
    optionsRect.y = optionsButtonRect.y + (buttonHeight - optionsRect.h) / 2;
    exitRect.x = (SCREEN_WIDTH - exitRect.w) / 2;
    exitRect.y = exitButtonRect.y + (buttonHeight - exitRect.h) / 2;

    // --- Loop do Menu ---
    int running_menu = 1;
    SDL_Event event;

    while (running_menu) {
        // Eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                return STATE_EXIT;
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
                        if (selectedOption == 0) { return STATE_PLAYING; }
                        else if (selectedOption == 1) { return STATE_LEADERBOARD; }
                        else if (selectedOption == 2) { return STATE_OPTIONS; }
                        else if (selectedOption == 3) { return STATE_EXIT; }
                        break;
                }
            }
        }

        // Renderização
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);

        SDL_Color playColor = (selectedOption == 0) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, playColor.r, playColor.g, playColor.b, playColor.a);
        SDL_RenderFillRect(renderer, &playButtonRect);
        SDL_RenderTexture(renderer, playTexture, NULL, &playRect);

        SDL_Color boardColor = (selectedOption == 1) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, boardColor.r, boardColor.g, boardColor.b, boardColor.a);
        SDL_RenderFillRect(renderer, &boardButtonRect);
        SDL_RenderTexture(renderer, boardTexture, NULL, &boardRect);

        SDL_Color optionsColor = (selectedOption == 2) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, optionsColor.r, optionsColor.g, optionsColor.b, optionsColor.a);
        SDL_RenderFillRect(renderer, &optionsButtonRect);
        SDL_RenderTexture(renderer, optionsTexture, NULL, &optionsRect);

        SDL_Color exitColor = (selectedOption == 3) ? selectedColor : buttonColor;
        SDL_SetRenderDrawColor(renderer, exitColor.r, exitColor.g, exitColor.b, exitColor.a);
        SDL_RenderFillRect(renderer, &exitButtonRect);
        SDL_RenderTexture(renderer, exitTexture, NULL, &exitRect);

        SDL_RenderPresent(renderer);
    }

    // Limpeza
    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(playTexture);
    SDL_DestroyTexture(boardTexture);
    SDL_DestroyTexture(optionsTexture);
    SDL_DestroyTexture(exitTexture);
    
    return STATE_EXIT;
}


// #########################################
// ### FUNÇÃO PRINCIPAL (main)           ###
// #########################################

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { return 1; }
    if (TTF_Init() == -1) { return 1; }

    GameContext context;
    // --- NOVO: Tela 1280x720 ---
    context.window = SDL_CreateWindow("Adedonha (Stop!) - Projeto AED", 1280, 720, 0);
    context.renderer = SDL_CreateRenderer(context.window, NULL);
    context.leaderboard = NULL; 

    // --- NOVO: Fontes Maiores ---
    context.font_title = TTF_OpenFont("font.ttf", 48); // Fonte maior
    context.font_body = TTF_OpenFont("font.ttf", 30);  // Fonte de corpo maior
    
    if (!context.font_title || !context.font_body) {
        SDL_Log("Erro fatal: Nao foi possivel carregar 'font.ttf'.");
        return 1;
    }
    
    // --- LÓGICA DE LETRAS CORRIGIDA: Começa com 26 ativadas ---
    for (int i = 0; i < 26; i++) {
        context.isLetterEnabled[i] = 1; // 1 = Ativado
    }

    // Popula o placar
    updateScore(&(context.leaderboard), "CPU 1", 50);
    updateScore(&(context.leaderboard), "Jogador", 20);
    updateScore(&(context.leaderboard), "CPU 2", 80);

    // O Loop de Estado Principal
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
                // O loop vai parar
                break;
        }
    }

    // Limpeza Global
    freeList(&(context.leaderboard));
    TTF_CloseFont(context.font_title);
    TTF_CloseFont(context.font_body);
    TTF_Quit();
    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    SDL_Quit();

    return 0;
}