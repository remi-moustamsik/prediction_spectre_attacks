#include <stdio.h>
#include <stdint.h>

#define ARRAY_SIZE 32
#define NUM_RUNS   1000   // nombre de tris répétés pour charger un peu le CPU

/* Échange de deux entiers */
static inline void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Quicksort en place sur un tableau d'int */
void qsort_int(int *arr, int left, int right) {
    if (left >= right) {
        return;
    }

    int i = left;
    int j = right;
    int pivot = arr[(left + right) / 2];

    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        if (i <= j) {
            swap(&arr[i], &arr[j]);
            i++;
            j--;
        }
    }

    if (left < j) {
        qsort_int(arr, left, j);
    }
    if (i < right) {
        qsort_int(arr, i, right);
    }
}

/* Affiche un tableau d'entiers */
void print_array(const int *arr, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    int arr[ARRAY_SIZE];

    // Génère un tableau déterministe (pseudo-aléatoire simple)
    for (int i = 0; i < ARRAY_SIZE; i++) {
        arr[i] = (i * 37 + 13) % 100;  // valeurs entre 0 et 99
    }

    printf("=== QSort benchmark (mode normal) ===\n");
    printf("Tableau initial :\n");
    print_array(arr, ARRAY_SIZE);

    // Premier tri (celui qu'on affiche)
    qsort_int(arr, 0, ARRAY_SIZE - 1);

    printf("Tableau trié :\n");
    print_array(arr, ARRAY_SIZE);

    // On recommence le tri plusieurs fois pour faire un benchmark
    // (sans réafficher à chaque fois pour ne pas polluer les stats avec l'I/O)
    for (int run = 0; run < NUM_RUNS; run++) {
        // On régénère le tableau dans l'état "non trié"
        for (int i = 0; i < ARRAY_SIZE; i++) {
            arr[i] = (i * 37 + 13) % 100;
        }
        qsort_int(arr, 0, ARRAY_SIZE - 1);
    }

    printf("Dernier élément (check) : %d\n", arr[ARRAY_SIZE - 1]);
    printf("Fin du programme qsort.\n");

    return 0;
}
