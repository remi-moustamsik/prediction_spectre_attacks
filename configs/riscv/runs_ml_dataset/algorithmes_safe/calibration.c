#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

// -----------------------------------------------------
// 1. FONCTIONS DE BAS NIVEAU RISC-V & INTRINSICS
// -----------------------------------------------------

// Lecture du Compteur de Cycles (rdcycle)
static inline uint64_t read_cycle_counter() {
    uint64_t cycle;
    __asm__ volatile ("rdcycle %0" : "=r" (cycle));
    return cycle;
}

// Instruction de barrière (Fence) pour sérialiser les accès.
// 'iorw,iorw' assure la sérialisation des accès mémoire et des instructions.
static inline void fence_serialise() {
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
}

// -----------------------------------------------------
// 2. DÉFINITIONS GLOBALES
// -----------------------------------------------------

#define CACHE_PROBE_SIZE 256
#define PROBE_OFFSET 4096
// Facteur de pollution stable, légèrement élevé pour éjecter le L2
#define POLLUTION_FACTOR 15

// Réduction des échantillons pour la stabilité en mode simulation
const int NUM_SAMPLES = 1000;

// Le tableau est dimensionné pour accueillir l'éjection L2 (environ 15MB)
uint8_t probe_array[CACHE_PROBE_SIZE * POLLUTION_FACTOR * PROBE_OFFSET];


// -----------------------------------------------------
// 3. FONCTION PRINCIPALE (Calibration)
// -----------------------------------------------------

int main() {
    uint64_t start, end;
    uint64_t min_hit_time = ULLONG_MAX;
    uint64_t min_miss_time = ULLONG_MAX;

    printf("--- Début de la Calibration (N=%d) ---\n", NUM_SAMPLES);

    // --- PHASE A : CALIBRATION DU HIT TIME (L1) ---
    // Pré-chargement de la ligne en cache L1
    volatile int temp_hit_load = probe_array[0];

    for (int i = 0; i < NUM_SAMPLES; i++) {

        fence_serialise(); // Sérialiser avant la mesure
        start = read_cycle_counter();

        volatile int temp_hit = probe_array[0]; // Accès rapide (Hit L1)

        fence_serialise(); // Sérialiser après l'accès
        end = read_cycle_counter();

        uint64_t time = end - start;
        if (time < min_hit_time) {
            min_hit_time = time;
        }
    }
    printf("--> Temps MINIMAL de HIT (T_hit) : %llu cycles\n", (unsigned long long)min_hit_time);


    // --- PHASE B : CALIBRATION DU MISS TIME (L2/DRAM) ---
    for (int i = 0; i < NUM_SAMPLES; i++) {

        // 1. ÉJECTION (Pollution) : Vide le cache L1 et L2
        for (int j = 0; j < CACHE_PROBE_SIZE * POLLUTION_FACTOR; j++) {
            volatile int temp_pollute = probe_array[j * PROBE_OFFSET];
        }

        fence_serialise(); // Sérialiser après la pollution
        start = read_cycle_counter();

        volatile int temp_miss = probe_array[PROBE_OFFSET]; // Accès lent (Miss L2/DRAM)

        fence_serialise(); // Sérialiser après l'accès
        end = read_cycle_counter();

        uint64_t time = end - start;
        if (time < min_miss_time) {
            min_miss_time = time;
        }
    }

    printf("--> Temps MINIMAL de MISS (T_miss) : %llu cycles\n\n", (unsigned long long)min_miss_time);


    // --- PHASE C : SUGGESTION DU SEUIL ---
    if (min_miss_time > min_hit_time * 2) {
        uint64_t threshold = min_hit_time + (min_miss_time - min_hit_time) / 2;
        printf("--- SUGGESTION DE SEUIL ---\n");
        printf("Ratio T_miss/T_hit : %.2f.\n", (double)min_miss_time / min_hit_time);
        printf("Seuil suggéré (valeur médiane) : %llu cycles\n", (unsigned long long)threshold);
        printf("Utilisez %llu comme 'hit_threshold' d'attaque (90%% du seuil).\n", (unsigned long long)(threshold * 0.9));
    } else {
        printf("!!! ÉCHEC DE CALIBRATION !!!\n");
        printf("Ratio T_miss/T_hit : %.2f. Requis : > 2.0. Vérifiez les latences L2/DRAM.\n", (double)min_miss_time / min_hit_time);
    }

    return 0;
}
