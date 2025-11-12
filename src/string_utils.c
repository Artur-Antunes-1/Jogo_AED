#include "string_utils.h"

#include <string.h>

static void swapStrings(const char** a, const char** b) {
    const char* temp = *a;
    *a = *b;
    *b = temp;
}

static int partition(const char* arr[], int low, int high) {
    const char* pivot = arr[high];
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        if (strcmp(arr[j], pivot) <= 0) {
            i++;
            swapStrings(&arr[i], &arr[j]);
        }
    }

    swapStrings(&arr[i + 1], &arr[high]);
    return i + 1;
}

void quickSortStrings(const char* arr[], int low, int high) {
    if (low < high) {
        int pivotIndex = partition(arr, low, high);
        quickSortStrings(arr, low, pivotIndex - 1);
        quickSortStrings(arr, pivotIndex + 1, high);
    }
}

