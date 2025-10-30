# Jogo em C com SDL3 (Projeto de Faculdade)

Este é um projeto desenvolvido para a disciplina **Algoritmos e Estruturas de Dados** com o objetivo de criar um jogo 2D em linguagem C, utilizando a biblioteca [SDL3](https://libsdl.org/) para a interface gráfica e (eventualmente) IA.

---

## 🎯 Status Atual

Atualmente, o projeto implementa as seguintes funcionalidades:

* **Menu Principal:** Navegável com setas e Enter, com opções para "Iniciar Jogo", "Ver Placar", "Opções de Letras" e "Sair".
* **Tela de Jogo:**
    * Sorteia uma letra aleatória (considerando as letras ativadas nas opções) e 5 temas (ordenados alfabeticamente com Quicksort).
    * Exibe um timer de 60 segundos.
    * Permite ao jogador digitar respostas nos campos de texto.
    * Navegação entre campos implementada com `TAB` (utilizando uma **Lista Circular Encadeada** internamente).
    * Suporte a `BACKSPACE`.
    * Finalização da rodada com `ENTER` ("STOP!") ou pelo fim do tempo.
    * Volta ao menu com `ESC`.
* **Tela de Opções:** Permite ativar/desativar individualmente cada uma das 26 letras do alfabeto para o sorteio.
* **Tela de Placar:** Exibe um placar de líderes (Top 5), gerenciado por uma **Lista Duplamente Encadeada Ordenada**.
* **Tela de Pontuação:** (Placeholder) Exibe as respostas da rodada e uma pontuação simulada, atualizando o placar.
* **Interface:** Resolução de 1280x720 com layout organizado e uso de duas fontes.

---

## 🚀 Pré-requisitos (Requirements)

Para compilar e rodar este projeto, você precisará de:

1.  **Um Compilador C:** O projeto foi desenvolvido e testado com `GCC (MinGW-w64)` no Windows.
2.  **Bibliotecas SDL3 e SDL3_ttf:** As bibliotecas de desenvolvimento (`-devel`) são necessárias. Este repositório *inclui* os arquivos de header (`/lib/include`), link (`/lib/lib`) e runtime (`/lib/bin`) necessários.
3.  **Um Arquivo de Fonte:** É necessário um arquivo de fonte TrueType (`.ttf`) chamado `font.ttf` na pasta raiz do projeto (ex: Roboto, Open Sans).

---

## 🛠️ Como Compilar

1.  Clone este repositório:
    ```bash
    git clone [https://github.com/Artur-Antunes-1/Jogo_AED.git](https://github.com/Artur-Antunes-1/Jogo_AED.git)
    ```
2.  Navegue até a pasta raiz do projeto.
3.  Execute o seguinte comando no seu terminal para compilar:

    ```bash
    gcc src/main.c src/cJSON.c -o build/meujogo.exe -Ilib/include -Llib/lib -lSDL3 -lSDL3_ttf -lcurl -lm -mwindows
    ```

---

## 🏃 Como Rodar

Após a compilação, o `meujogo.exe` estará na pasta `/build`. O jogo precisa das DLLs das bibliotecas para rodar.

1.  **Copie os arquivos `SDL3.dll`, `SDL3_ttf.dll` e `libcurl-x64.dll`** da pasta `/lib/bin/`.
2.  **Cole-os** na sua pasta `/build/` (ao lado do `meujogo.exe` que você acabou de compilar).
3.  Certifique-se de que o arquivo `font.ttf` está na pasta raiz (`JogoC&SDL/`).
4.  Execute o jogo (pelo terminal ou dando dois cliques):

    ```bash
    ./build/meujogo.exe
    ```

---

## 🎮 Controles

**Menu Principal / Opções / Placar:**
* **Setas Cima/Baixo:** Navegar entre as opções.
* **Enter:** Selecionar opção / Ativar/Desativar letra.
* **ESC:** Voltar ao menu anterior (nas telas de Opções e Placar).

**Durante o Jogo:**
* **Digitar:** Preencher as respostas.
* **Backspace:** Apagar o último caractere.
* **TAB:** Mover para o próximo campo de resposta (circular).
* **Enter:** Finalizar a rodada ("STOP!").
* **ESC:** Cancelar a rodada e voltar ao Menu Principal.

**Geral:**
* **Fechar Janela (X):** Encerrar o jogo.

---

## 📁 Estrutura do Projeto

```plaintext
JogoC&SDL/
├── .gitignore       # Arquivos a serem ignorados pelo Git
├── README.md        # Este arquivo
├── font.ttf         # Arquivo de fonte necessário
├── src/
│   └── main.c       # Código-fonte principal do jogo
├── lib/
│   ├── bin/
│   │   ├── SDL3.dll       # DLL principal do SDL
│   │   └── SDL3_ttf.dll   # DLL da biblioteca de fontes
│   │   └── libcurl-x64.dll
│   ├── include/
│   │   └── SDL3/          # Arquivos de cabeçalho (.h) do SDL e SDL_ttf
│   └── lib/
│       └── ...            # Arquivos de link (.a) do SDL e SDL_ttf
└── build/               # (Ignorada) Onde os executáveis são compilados
```

---

## 📄 Licença

Este projeto é para fins educacionais.