
Jogo em C com SDL3 (Projeto de Faculdade)
=========================================

Este é um projeto desenvolvido para a disciplina **Algoritmos e Estruturas de Dados** com o objetivo de criar um jogo 2D em linguagem C, utilizando a biblioteca [SDL3](https://libsdl.org/ "null") para a interface gráfica e a API do Google Gemini para IA.

🎯 Status Atual
---------------

Atualmente, o projeto implementa as seguintes funcionalidades:

-   **Menu Principal:** Navegável com setas e Enter, com opções para "Iniciar Jogo", "Ver Placar", "Opções de Letras" e "Sair".

-   **Integração com IA (Google Gemini):**

    -   **Temas Dinâmicos:** A IA gera 5 temas criativos e adequados para a letra sorteada no início de cada rodada.

    -   **Juiz de IA:** A IA valida as respostas do jogador na tela de pontuação, atribuindo pontuação real (10 para acertos, 0 para erros).

-   **Tela de Jogo:**

    -   Sorteia uma letra aleatória (considerando as letras ativadas nas opções).

    -   Exibe um timer de 60 segundos.

    -   Permite ao jogador digitar respostas nos campos de texto.

    -   Navegação entre campos implementada com `TAB` (utilizando uma **Lista Circular Encadeada** internamente).

    -   Suporte a `BACKSPACE`.

    -   Finalização da rodada com `ENTER` ("STOP!") ou pelo fim do tempo.

*   **Tela de Opções:** Permite ativar/desativar individualmente cada uma das 26        letras do alfabeto para o sorteio.

*   **Tela de Placar:** Exibe um placar de líderes (Top 5), gerenciado por uma  **Lista Duplamente Encadeada Ordenada**.

* **Tela de Pontuação:** (Placeholder) Exibe as respostas da rodada e uma pontuação simulada, atualizando o placar.

* **Interface:** Resolução de 1280x720 com layout organizado e uso de duas fontes.

-   **Estruturas de Dados:**

    -   **Lista Duplamente Encadeada Ordenada:** Usada para o Placar de Líderes (`PlayerNode`).

    -   **Lista Circular Encadeada:** Usada para a navegação entre os campos de input (`InputNode`).

    -   **Quicksort:** Usado para ordenar os temas recebidos da IA.

-   **Gerenciamento de Rede e JSON:**

    -   Uso da `libcurl` para fazer as chamadas HTTP para a API do Google.

    -   Uso da `cJSON` para montar o *payload* da requisição e ler a resposta da IA.



## 🚀 Pré-requisitos (Requirements)

Para compilar e rodar este projeto, você precisará de:

1.  **Um Compilador C:** O projeto foi desenvolvido e testado com `GCC (MinGW-w64)` no Windows.
2.  **Bibliotecas SDL3 e SDL3_ttf:** As bibliotecas de desenvolvimento (`-devel`) são necessárias. Este repositório *inclui* os arquivos de header (`/lib/include`), link (`/lib/lib`) e runtime (`/lib/bin`) necessários.
3.  **Um Arquivo de Fonte:** É necessário um arquivo de fonte TrueType (`.ttf`) chamado `font.ttf` na pasta raiz do projeto (ex: Roboto, Open Sans).



🛠️ Como Compilar e Rodar (Windows)
-----------------------------------

Siga estes passos para compilar o projeto em um ambiente Windows.

### 1\. Pré-requisitos (Ambiente)

Este projeto requer um compilador GCC. A forma mais fácil de instalá-lo no Windows é através do **MSYS2**:

1.  Baixe e instale o [MSYS2](https://www.msys2.org/ "null").

2.  Após a instalação, abra o terminal "MSYS2 UCRT 64-bit".

3.  Instale o toolchain de compilação GCC (MinGW-w64) com o comando:

    ```
    pacman -S mingw-w64-ucrt-x86_64-gcc

    ```

As bibliotecas (SDL3, SDL3_ttf, libcurl) já estão incluídas na pasta `lib/` para Windows (MinGW).

### 2\.  Clone este repositório:
    
    git clone https://github.com/Artur-Antunes-1/Jogo_AED.git

### 3\. Chave de API (Obrigatório)

O jogo utiliza a API do Google Gemini e requer uma chave de API para funcionar.

1.  Acesse o [Google AI Studio](https://aistudio.google.com/app "null") e gere sua própria chave de API gratuita.

2.  Na pasta `src/` do projeto, **crie um novo arquivo** chamado `config.h`.

3.  Adicione o seguinte conteúdo a esse arquivo, substituindo `"SUA_CHAVE_DE_API_VAI_AQUI"` pela chave que você gerou:

    ```
    #ifndef CONFIG_H
    #define CONFIG_H

    /*
     * Substitua a string abaixo pela sua chave de API real
     * que você obteve do Google AI Studio.
     */
    #define API_KEY "SUA_CHAVE_DE_API_VAI_AQUI"

    #endif // CONFIG_H

    ```

### 4\. Compilando o Projeto

O compilador não cria pastas automaticamente. Você precisa criar a pasta `build` manualmente.

1.  Abra um terminal (como PowerShell ou CMD) na raiz do projeto.

2.  **Crie a pasta `build`**:

    ```
    mkdir build

    ```

3.  **Execute o comando de compilação**:

    ```
    gcc src/main.c src/cJSON.c -o build/meujogo.exe -Ilib/include -Llib/lib -lSDL3 -lSDL3_ttf -lcurl -lm -mwindows

    ```

### 5\. Executando o Jogo

O executável `meujogo.exe` precisa dos arquivos `.dll` e da fonte para funcionar.

1.  **Copie as DLLs** da pasta `lib/bin/` para a pasta `build/`:

    -   `lib/bin/SDL3.dll` -> `build/SDL3.dll`

    -   `lib/bin/SDL3_ttf.dll` -> `build/SDL3_ttf.dll`

    -   `lib/bin/libcurl-x64.dll` -> `build/libcurl-x64.dll` *(Você também precisará dos arquivos `zlib1.dll`, `libfreetype-6.dll`, etc., se eles forem dependências no seu sistema. As DLLs do curl já devem incluir o necessário para SSL).*

2.  **Copie a fonte** do projeto para a pasta `build/`:

    -   `font.ttf` -> `build/font.ttf`

    Sua pasta `build/` agora está autossuficiente e deve se parecer com isto:

```
build/
├── meujogo.exe
├── SDL3.dll
├── SDL3_ttf.dll
├── libcurl.dll
└── font.ttf

```

3.  Agora, sua pasta `build/` deve conter `meujogo.exe`, `font.ttf` e os arquivos `.dll`. Você pode executar o jogo:

    ```
    .\build\meujogo.exe

    ```


🎮 Controles
------------

**Menu Principal / Opções / Placar:**

-   **Setas Cima/Baixo:** Navegar entre as opções.

-   **Enter:** Selecionar opção / Ativar/Desativar letra.

-   **ESC:** Voltar ao menu anterior (nas telas de Opções e Placar).

**Durante o Jogo:**

-   **Digitar:** Preencher as respostas.

-   **Backspace:** Apagar o último caractere.

-   **TAB:** Mover para o próximo campo de resposta (circular).

-   **Enter:** Finalizar a rodada ("STOP!").

-   **ESC:** Cancelar a rodada e voltar ao Menu Principal.

**Geral:**

-   **Fechar Janela (X):** Encerrar o jogo.

📁 Estrutura do Projeto
-----------------------

```
Jogo_AED/
├── .gitignore
├── README.md
├── font.ttf         # Arquivo de fonte
├── src/
│   ├── main.c       # Código-fonte principal
│   ├── cJSON.c      # Código da biblioteca JSON
│   ├── cJSON.h      # Header da biblioteca JSON
│   └── config.h     # (Ignorado) Contém a API_KEY
├── lib/
│   ├── bin/         # DLLs para Windows
│   │   ├── SDL3.dll
│   │   ├── SDL3_ttf.dll
│   │   └── libcurl-x64.dll
│   ├── include/     # Headers (.h) das bibliotecas
│   └── lib/         # Bibliotecas de link (.a) para MinGW
└── build/           # (Ignorado) Pasta de saída do executável
    └── meujogo.exe

```

---

## 📄 Licença

Este projeto é para fins educacionais.