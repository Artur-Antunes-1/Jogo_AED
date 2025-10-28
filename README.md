# Jogo em C com SDL3 (Projeto de Faculdade)

Este é um projeto desenvolvido para a disciplina **Algoritmos e Estruturas de Dados** com o objetivo de criar um jogo 2D em linguagem C, utilizando a biblioteca [SDL3](https://libsdl.org/) para a interface gráfica e (eventualmente) IA.

---

## 🎯 Status Atual

Atualmente, o projeto renderiza uma janela gráfica de 800x600 e exibe um objeto (jogador) estático no centro. Esta é a base para a implementação de movimento e IA.

---

## 🚀 Pré-requisitos (Requirements)

Para compilar e rodar este projeto, você precisará de:

1.  **Um Compilador C:** O projeto foi desenvolvido e testado com `GCC (MinGW-w64)` no Windows.
2.  **Biblioteca SDL3:** A biblioteca de desenvolvimento (`-devel`) do SDL3 é necessária. Este repositório *inclui* os arquivos de header (`/lib/include`) e de link (`/lib/lib`) necessários na pasta `/lib`.

---

## 🛠️ Como Compilar

1.  Clone este repositório:
    ```bash
    git clone https://github.com/Artur-Antunes-1/Jogo_AED.git
    ```
2.  Navegue até a pasta raiz do projeto.
3.  Execute o seguinte comando no seu terminal para compilar:

    ```bash
    gcc src/main.c -o build/meujogo.exe -Ilib/include -Llib/lib -lSDL3 -mwindows
    ```

---

## 🏃 Como Rodar

Após a compilação, o `meujogo.exe` estará na pasta `/build`. O jogo também precisa do arquivo `SDL3.dll` para rodar, que já está incluído no repositório.

1.  **Copie o arquivo `SDL3.dll`** da pasta `/lib/bin/`.
2.  **Cole-o** na sua pasta `/build/` (ao lado do `meujogo.exe` que você acabou de compilar).
3.  Execute o jogo (pelo terminal ou dando dois cliques):

    ```bash
    ./build/meujogo.exe
    ```

---

## 🎮 Controles

* **(A ser implementado)**
* **Fechar Janela:** Clique no "X" da janela.

---

## 📁 Estrutura do Projeto

```plaintext
JogoC&SDL/
├── .gitignore   # Arquivos a serem ignorados pelo Git
├── README.md    # Este arquivo
├── src/
│   └── main.c   # Código-fonte principal do jogo
├── lib/
│   ├── bin/
│   │   └── SDL3.dll # DLL necessária para rodar o jogo
│   ├── include/
│   │   └── SDL3/    # Arquivos de cabeçalho (.h) do SDL3
│   └── lib/
│       └── ...      # Arquivos de link (.a) do SDL3
└── build/           # (Ignorada) Onde os executáveis são compilados
```

---

## 📄 Licença

Este projeto é para fins educacionais.