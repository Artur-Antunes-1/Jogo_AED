#include <SDL3/SDL.h>

int main(int argc, char* argv[]) {
    // Inicializa o SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Erro ao inicializar SDL: %s", SDL_GetError());
        return 1;
    }

    // Cria a janela
    // (Título, Largura, Altura, Flags)
    SDL_Window* window = SDL_CreateWindow("Meu Jogo SDL3!", 800, 600, 0);
    if (!window) {
        SDL_Log("Erro ao criar janela: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Cria o renderizador (para desenhar na janela)
    // (Janela, Driver, Flags)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Erro ao criar renderizador: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event event;

    // DEFININDO O JOGADOR
    SDL_FRect player;
    player.x = 400.0f; // Posição X
    player.y = 300.0f; // Posição Y
    player.w = 50.0f; // Largura (Width)
    player.h = 50.0f; // Altura (Height)

    // ----- O GAME LOOP (Loop Principal do Jogo) -----
    while (running) {
        // 1. Processar Eventos (Input)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { // Clicou no "X" da janela
                running = 0;
            }
        }

        // 2. Lógica do Jogo (Update)
        // ... (por enquanto, nada) ...

        // 3. Renderização (Desenho)
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // Cor de fundo (cinza escuro)
        SDL_RenderClear(renderer);                         // Limpa a tela

        // ... (aqui é onde você desenharia o jogador, inimigos, etc.) ...
        
        // --- DESENHO DO JOGADOR ---
        // Definindo cor do desenho para branco (R, G, B, Alpha)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // Desenha o retângulo do jogador
        SDL_RenderFillRect(renderer, &player);

        SDL_RenderPresent(renderer); // Mostra o que foi desenhado na tela
    }
    // ----- Fim do Game Loop -----

    // Limpeza
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}