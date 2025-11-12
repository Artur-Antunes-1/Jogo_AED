#include "leaderboard.h"

#include <stdlib.h>
#include <string.h>

PlayerNode* createPlayer(const char* name) {
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (newNode) {
        if (name) {
            strncpy(newNode->name, name, sizeof(newNode->name) - 1);
            newNode->name[sizeof(newNode->name) - 1] = '\0';
        } else {
            newNode->name[0] = '\0';
        }
        newNode->totalScore = 0;
        newNode->next = NULL;
        newNode->prev = NULL;
    }
    return newNode;
}

void insertSorted(PlayerNode** head, PlayerNode* newNode) {
    if (!head || !newNode) {
        return;
    }

    if (*head == NULL || (*head)->totalScore <= newNode->totalScore) {
        newNode->next = *head;
        if (*head != NULL) {
            (*head)->prev = newNode;
        }
        *head = newNode;
    } else {
        PlayerNode* current = *head;
        while (current->next != NULL && current->next->totalScore > newNode->totalScore) {
            current = current->next;
        }
        newNode->next = current->next;
        if (current->next != NULL) {
            current->next->prev = newNode;
        }
        current->next = newNode;
        newNode->prev = current;
    }
}

PlayerNode* findByName(PlayerNode* head, const char* name) {
    PlayerNode* current = head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void removePlayer(PlayerNode** head, PlayerNode* playerToRemove) {
    if (!head || !*head || !playerToRemove) {
        return;
    }

    if (*head == playerToRemove) {
        *head = playerToRemove->next;
    }
    if (playerToRemove->prev != NULL) {
        playerToRemove->prev->next = playerToRemove->next;
    }
    if (playerToRemove->next != NULL) {
        playerToRemove->next->prev = playerToRemove->prev;
    }

    playerToRemove->prev = NULL;
    playerToRemove->next = NULL;
}

void freeLeaderboard(PlayerNode** head) {
    if (!head) {
        return;
    }

    PlayerNode* current = *head;
    while (current != NULL) {
        PlayerNode* nextNode = current->next;
        free(current);
        current = nextNode;
    }
    *head = NULL;
}

void updateScore(PlayerNode** head, const char* name, int pointsToAdd) {
    if (!head || !name) {
        return;
    }

    PlayerNode* player = findByName(*head, name);
    if (player == NULL) {
        player = createPlayer(name);
    } else {
        removePlayer(head, player);
    }

    if (player != NULL) {
        player->totalScore += pointsToAdd;
        insertSorted(head, player);
    }
}

