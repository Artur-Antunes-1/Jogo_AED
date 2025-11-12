#ifndef LEADERBOARD_H
#define LEADERBOARD_H

typedef struct PlayerNode {
    char name[50];
    int totalScore;
    struct PlayerNode* next;
    struct PlayerNode* prev;
} PlayerNode;

PlayerNode* createPlayer(const char* name);
void insertSorted(PlayerNode** head, PlayerNode* newNode);
PlayerNode* findByName(PlayerNode* head, const char* name);
void removePlayer(PlayerNode** head, PlayerNode* playerToRemove);
void freeLeaderboard(PlayerNode** head);
void updateScore(PlayerNode** head, const char* name, int pointsToAdd);

#endif /* LEADERBOARD_H */

