#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

// --- NOVO INCLUDE: Para 'strncasecmp' (ignorar maiúsculas/minúsculas) ---
#include <strings.h> 

// --- NOVOS INCLUDES PARA IA ---
#include "config.h"     // Para a API_KEY (src/config.h)
#include <curl/curl.h>  // Para libcurl (rede)
#include "cJSON.h"      // Para cJSON (ler a resposta)

// --- Constantes de Tela ---
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

// ######################################################
// ### FUNÇÕES DE REDE E IA (LIBCURL + CJSON)         ###
// ######################################################

// Estrutura para guardar a resposta da web
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Função de callback que o cURL usa para escrever os dados recebidos na memória
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("Erro: falha ao alocar memória (realloc)\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

// A função principal da IA!
char* call_gemini_api(const char* prompt) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    char* response_text = NULL;

    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if(!curl) {
        fprintf(stderr, "Erro ao iniciar o cURL\n");
        free(chunk.memory);
        return NULL;
    }

    char api_url[200];
    sprintf(api_url, "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%s", API_KEY);

    cJSON *json_payload = cJSON_CreateObject();
    cJSON *contents = cJSON_CreateArray();
    cJSON *part_obj = cJSON_CreateObject();
    cJSON *parts_array = cJSON_CreateArray();
    cJSON *text_obj = cJSON_CreateObject();
    
    cJSON_AddStringToObject(text_obj, "text", prompt);
    cJSON_AddItemToArray(parts_array, text_obj);
    cJSON_AddItemToObject(part_obj, "parts", parts_array);
    cJSON_AddItemToArray(contents, part_obj);
    cJSON_AddItemToObject(json_payload, "contents", contents);
    
    char *json_string = cJSON_Print(json_payload);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Bypass do SSL

    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() falhou: %s\n", curl_easy_strerror(res));
    } else {
        // (Opcional) Imprime a resposta bruta para depuração
        // printf("\n--- RESPOSTA JSON BRUTA DO SERVIDOR ---\n%s\n---------------------------------------\n\n", chunk.memory);
        
        cJSON *json_response = cJSON_Parse(chunk.memory);
        if (json_response == NULL) {
            fprintf(stderr, "Erro ao analisar JSON: %s\n", cJSON_GetErrorPtr());
        } else {
            cJSON *error = cJSON_GetObjectItem(json_response, "error");
            if (error) {
                cJSON *errorMessage = cJSON_GetObjectItem(error, "message");
                if (cJSON_IsString(errorMessage)) {
                    fprintf(stderr, "ERRO DA API: %s\n", errorMessage->valuestring);
                }
            } else {
                cJSON *candidates = cJSON_GetObjectItem(json_response, "candidates");
                if (cJSON_IsArray(candidates)) {
                    cJSON *candidate = cJSON_GetArrayItem(candidates, 0);
                    if (candidate) {
                        cJSON *content = cJSON_GetObjectItem(candidate, "content");
                        cJSON *parts = cJSON_GetObjectItem(content, "parts");
                        if (cJSON_IsArray(parts)) {
                            cJSON *part = cJSON_GetArrayItem(parts, 0);
                            cJSON *text = cJSON_GetObjectItem(part, "text");
                            if (cJSON_IsString(text) && (text->valuestring != NULL)) {
                                response_text = strdup(text->valuestring);
                            }
                        }
                    }
                }
            }
            cJSON_Delete(json_response);
        }
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(json_string);
    cJSON_Delete(json_payload);
    free(chunk.memory);

    return response_text;
}

// --- Função de diagnóstico (pode ser removida mais tarde) ---
void list_available_models() {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    printf("Verificando modelos de IA disponíveis...\n");
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;
    curl = curl_easy_init();
    if(!curl) { /* ... */ return; }

    char api_url[200];
    sprintf(api_url, "https://generativelanguage.googleapis.com/v1beta/models?key=%s", API_KEY);
    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        fprintf(stderr, "list_models() falhou: %s\n", curl_easy_strerror(res));
    } else {
        printf("\n--- LISTA DE MODELOS DISPONÍVEIS (JSON) ---\n%s\n----------------------------------------------\n\n", chunk.memory);
    }
    curl_easy_cleanup(curl);
    free(chunk.memory);
}

// --- LÓGICA DO ESTADO: JOGANDO (CORRIGIDO) ---
GameState runPlaying(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;

    const Uint64 ROUND_DURATION_MS = 60000;
    
    // --- Variáveis do Timer (inicializadas mais tarde) ---
    Uint64 startTime;
    SDL_Texture* timerTexture = NULL;
    SDL_FRect timerRect;
    char timerText[20];
    int lastSecond = 61; // Força a atualização no primeiro frame

    // --- Cores ---
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

    // ----- NOVO: IA GERA TEMAS -----
    char prompt[512]; // Aumenta o tamanho do prompt
    
    // --- CORREÇÃO (Problema 3): Prompt Melhorado ---
    sprintf(prompt, "Crie %d temas para um jogo de Adedonha (Stop!) com a letra %c. "
                    "IMPORTANTE: Para cada tema, garanta que exista pelo menos uma resposta óbvia e comum em português com a letra %c. "
                    "Não peça por temas impossíveis (como 'Oceano com K'). "
                    "Responda APENAS com os %d temas, separados por vírgula, sem quebra de linha, sem espaços extras. "
                    "Exemplo: Algo salgado,Animal que voa,Filme de terror,Cor,Objeto", 
                    NUM_THEMES, chosenLetter, chosenLetter, NUM_THEMES);
    
    // Mostra tela de "Carregando..."
    SDL_Texture* loadingTexture = NULL;
    SDL_FRect loadingRect;
    createTextTexture(context, 1, "Sorteando temas com a IA...", &loadingTexture, &loadingRect, 0, 300, white);
    loadingRect.x = (SCREEN_WIDTH - loadingRect.w) / 2;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, loadingTexture, NULL, &loadingRect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(loadingTexture);

    // Chama a API
    char* ai_response = call_gemini_api(prompt);

    if (ai_response == NULL) {
        // Mostra erro
        SDL_Texture* errTexture = NULL;
        SDL_FRect errRect;
        createTextTexture(context, 1, "Erro: Falha ao contatar a IA.", &errTexture, &errRect, 0, 300, red);
        errRect.x = (SCREEN_WIDTH - errRect.w) / 2;
        
        SDL_Texture* helpTexture = NULL;
        SDL_FRect helpRect;
        createTextTexture(context, 0, "Verifique sua API Key ou conexão.", &helpTexture, &helpRect, 0, 360, white);
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

    // Processa a resposta da IA
    const char* chosenThemes[NUM_THEMES];
    int themeCount = 0;
    char* token = strtok(ai_response, ","); 
    while (token != NULL && themeCount < NUM_THEMES) {
        while (*token == ' ') token++; // Remove espaços
        chosenThemes[themeCount] = token;
        themeCount++;
        token = strtok(NULL, ",");
    }
    for (int i = themeCount; i < NUM_THEMES; i++) { chosenThemes[i] = "Erro da IA"; }

    quickSortStrings(chosenThemes, 0, NUM_THEMES - 1);

    // ----- Preparação das Texturas e Inputs (Lista Circular) -----
    SDL_Texture* letterTexture = NULL;
    SDL_FRect letterRect;
    char letterText[30];
    sprintf(letterText, "Letra Sorteada: %c", chosenLetter);
    int topRowY = 50; // Y da linha superior
    createTextTexture(context, 1, letterText, &letterTexture, &letterRect, 50, topRowY, white);

    // --- CORREÇÃO (Problema 1): Layout Melhorado ---
    InputNode* headInput = NULL;
    InputNode* currentInput = NULL;
    InputNode* prevInput = NULL;

    float inputYStart = 150.0f;
    float inputHeight = 50.0f;
    float inputSpacing = 80.0f; // Mais espaço vertical
    float labelX = 50.0f;       // Labels mais à esquerda
    float inputX = 400.0f;      // Caixas de input mais à direita
    float inputWidth = 800.0f;  // Caixas mais largas
    
    // --- CORREÇÃO (Erro de Compilação anterior) ---
    float textPaddingY = (inputHeight - TTF_GetFontHeight(context->font_body)) / 2.0f; // Centraliza texto verticalmente

    for (int i = 0; i < NUM_THEMES; i++) {
        currentInput = (InputNode*)malloc(sizeof(InputNode));
        if (!currentInput) { return STATE_EXIT; }
        currentInput->next = NULL;

        InputField* field = &(currentInput->field);
        strcpy(field->text, "");
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

        if (headInput == NULL) { headInput = currentInput; }
        else { prevInput->next = currentInput; }
        prevInput = currentInput;
    }
    if (prevInput != NULL) { prevInput->next = headInput; }

    InputNode* activeNode = headInput;
    SDL_StartTextInput(context->window);

    // ----- Game Loop (Playing) -----
    
    // --- CORREÇÃO (Problema 2): Inicia o timer AGORA ---
    startTime = SDL_GetTicks();

    int running_playing = 1;
    SDL_Event event;
    GameState nextState = STATE_MENU; 

    if (activeNode == NULL) { running_playing = 0; }

    while (running_playing) {
        int textChanged = 0;
        // 1. Eventos (Lógica da Lista Circular)
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
                    activeNode->field.text[strlen(activeNode->field.text) - 1] = '\0';
                    textChanged = 1;
                }
                else if (event.key.key == SDLK_TAB) {
                    activeNode = activeNode->next;
                }
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (strlen(activeNode->field.text) + strlen(event.text.text) < MAX_INPUT_LENGTH) {
                    strcat(activeNode->field.text, event.text.text);
                    textChanged = 1;
                }
            }
        }

        // 2. Lógica (Update)
        Uint64 elapsedMs = SDL_GetTicks() - startTime; // Agora está correto
        int secondsLeft = (int)((ROUND_DURATION_MS - elapsedMs) / 1000);

        if (elapsedMs >= ROUND_DURATION_MS) {
            secondsLeft = 0;
            nextState = STATE_SCORING;
            running_playing = 0;
        }
        if (secondsLeft < 0) secondsLeft = 0; // Garante que não fique negativo

        if (secondsLeft != lastSecond) {
            sprintf(timerText, "Tempo: %d", secondsLeft);
            createTextTexture(context, 1, timerText, &timerTexture, &timerRect, 0, topRowY, white);
            timerRect.x = SCREEN_WIDTH - timerRect.w - 50; // Alinha à direita
            lastSecond = secondsLeft;
        }
        
        if (textChanged) {
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
                    cursorRect.x = field->inputBoxRect.x + 10;
                } else {
                    cursorRect.x = field->rect.x + field->rect.w + 5;
                }
                cursorRect.y = field->inputBoxRect.y + (field->inputBoxRect.h - 30) / 2;
                cursorRect.w = 10;
                cursorRect.h = 30;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &cursorRect);
            }
            tempNode = tempNode->next;
        } while (tempNode != headInput);
        
        SDL_RenderPresent(renderer);
    }

    // --- Salva os resultados LENDO a Lista Circular ---
    if (nextState == STATE_SCORING) {
        context->lastLetter = chosenLetter;
        InputNode* tempNode = headInput;
        int i = 0;
        do {
             strcpy(context->lastThemes[i], chosenThemes[i]);
             strcpy(context->lastAnswers[i], tempNode->field.text);
             tempNode = tempNode->next;
             i++;
        } while(tempNode != headInput && i < NUM_THEMES);
    }
    
    // --- Limpeza da Lista Circular ---
    SDL_StopTextInput(context->window);
    SDL_DestroyTexture(letterTexture);
    SDL_DestroyTexture(timerTexture); 
    
    InputNode* currentNode = headInput;
    if (currentNode) {
        InputNode* nextNode;
        InputNode* firstNode = headInput;
        currentNode = headInput;
        do {
            nextNode = currentNode->next;
            if (currentNode->field.texture) SDL_DestroyTexture(currentNode->field.texture);
            if (currentNode->field.labelTexture) SDL_DestroyTexture(currentNode->field.labelTexture);
            free(currentNode);
            currentNode = nextNode;
        } while (currentNode != firstNode);
    }
    
    // --- LIBERA A MEMÓRIA DA RESPOSTA DA IA ---
    free(ai_response);

    return nextState; 
}


// --- LÓGICA DO ESTADO: PONTUAÇÃO (COM IA JUIZ) ---
GameState runScoring(GameContext* context) {
    SDL_Renderer* renderer = context->renderer;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {200, 200, 200, 255};
    SDL_Color green = {50, 200, 50, 255}; // Cor para acerto
    SDL_Color red = {200, 50, 50, 255};   // Cor para erro

    // Mostra tela de "Carregando Pontuação..."
    SDL_Texture* loadingTexture = NULL;
    SDL_FRect loadingRect;
    createTextTexture(context, 1, "IA está julgando suas respostas...", &loadingTexture, &loadingRect, 0, 300, white);
    loadingRect.x = (SCREEN_WIDTH - loadingRect.w) / 2;
    SDL_SetRenderDrawColor(renderer, 50, 50, 80, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, loadingTexture, NULL, &loadingRect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(loadingTexture);


    // --- NOVO (Problema 3): Lógica de Pontuação Real (Batch) ---
    int scoreThisRound = 0;
    int scores[NUM_THEMES] = {0}; // Guarda a pontuação de cada tema
    
    // 1. Monta um prompt "batch" gigante
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
        // Se a resposta estiver vazia, nem pergunta para a IA
        if (strlen(context->lastAnswers[i]) == 0) {
            sprintf(temp_prompt, "Tema: '%s', Resposta: ''\n", context->lastThemes[i]);
        } else {
            sprintf(temp_prompt, "Tema: '%s', Resposta: '%s'\n", context->lastThemes[i], context->lastAnswers[i]);
        }
        strcat(validation_prompt, temp_prompt);
    }
    
    // 2. Chama a IA (apenas uma vez)
    char* ai_response = call_gemini_api(validation_prompt);
    
    // 3. Processa a resposta da IA (ex: "Sim,Sim,Nao,Sim,Nao")
    if (ai_response) {
        char* token = strtok(ai_response, ",");
        int i = 0;
        while (token != NULL && i < NUM_THEMES) {
            while (*token == ' ' || *token == '\n') token++; // Limpa espaços/quebra de linha
            
            // strncasecmp compara ignorando maiúsculas
            if (strncasecmp(token, "Sim", 3) == 0) {
                scores[i] = 10; // 10 pontos por acerto
                scoreThisRound += 10;
            } else {
                scores[i] = 0; // 0 pontos por erro
            }
            
            token = strtok(NULL, ",");
            i++;
        }
        free(ai_response);
    } else {
        // Se a IA falhar, dá 0 pontos por segurança
        scoreThisRound = 0;
        for (int i = 0; i < NUM_THEMES; i++) scores[i] = 0;
    }
    
    // Atualiza o placar
    updateScore(&(context->leaderboard), "Jogador", scoreThisRound);

    // --- Prepara texturas para esta tela ---
    SDL_Texture* titleTexture = NULL;
    SDL_FRect titleRect;
    char scoreText[100];
    sprintf(scoreText, "Você fez %d pontos nesta rodada! (Letra %c)", scoreThisRound, context->lastLetter);
    createTextTexture(context, 1, scoreText, &titleTexture, &titleRect, 0, 100, white);
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2; // Centraliza
    
    // Mostra as respostas com suas pontuações (Verde/Vermelho)
    SDL_Texture* answerTextures[NUM_THEMES];
    SDL_FRect answerRects[NUM_THEMES];
    for (int i = 0; i < NUM_THEMES; i++) {
        char answerLine[200];
        // Mostra a pontuação individual
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
        int yPos = 250;
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
            yPos += 50;
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
    const int spacingX = 150;
    const int spacingY = 80;
    const int startX = (SCREEN_WIDTH - (spacingX * (numCols - 1))) / 2;
    const int startY = 150;

    int running_options = 1;
    SDL_Event event;

    while (running_options) {
        // Eventos
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
    const int buttonWidth = 400;
    const int buttonHeight = 60;
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

    curl_global_init(CURL_GLOBAL_ALL);

    GameContext context;
    context.window = SDL_CreateWindow("Adedonha (Stop!) - Projeto AED", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    context.renderer = SDL_CreateRenderer(context.window, NULL);
    context.leaderboard = NULL; 

    context.font_title = TTF_OpenFont("font.ttf", 48); // Fonte maior
    context.font_body = TTF_OpenFont("font.ttf", 30);  // Fonte de corpo maior
    
    if (!context.font_title || !context.font_body) {
        SDL_Log("Erro fatal: Nao foi possivel carregar 'font.ttf'.");
        return 1;
    }
    
    for (int i = 0; i < 26; i++) {
        context.isLetterEnabled[i] = 1;
    }

    // Popula o placar
    updateScore(&(context.leaderboard), "CPU 1", 50);
    updateScore(&(context.leaderboard), "Jogador", 20); // Corrigido
    updateScore(&(context.leaderboard), "CPU 2", 80);

    // list_available_models(); // Chamada de diagnóstico (comentada)

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
    curl_global_cleanup();
    TTF_CloseFont(context.font_title);
    TTF_CloseFont(context.font_body);
    TTF_Quit();
    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    SDL_Quit();

    return 0;
}

