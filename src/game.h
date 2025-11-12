#ifndef GAME_H
#define GAME_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>

#define MAX_INPUT_LENGTH 50
#define NUM_THEMES 5
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_SCORING,
    STATE_LEADERBOARD,
    STATE_OPTIONS,
    STATE_EXIT
} GameState;

typedef struct PlayerNode PlayerNode;

typedef struct {
    SDL_Color bgColor;
    SDL_Color bgGradientEnd;
    SDL_Color textColor;
    SDL_Color titleColor;
    SDL_Color buttonColor;
    SDL_Color highlightColor;
    SDL_Color inputBgColor;
    SDL_Color accentGreen;
    SDL_Color accentRed;
    SDL_Color accentGray;
} AppColors;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_title;
    TTF_Font* font_body;
    PlayerNode* leaderboard;
    char lastLetter;
    char lastThemes[NUM_THEMES][100];
    char lastAnswers[NUM_THEMES][MAX_INPUT_LENGTH];
    int isLetterEnabled[26];
    AppColors colors;
} GameContext;

void init_default_colors(AppColors* colors);

#endif /* GAME_H */

