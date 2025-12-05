// configs/riscv/bench_spectre_like.c
// Micro-benchmark RISC-V "Spectre-like" purement éducatif :
// il montre comment un chemin conditionnel peut encod(er une valeur
// dans le cache et comment on peut la retrouver par mesure de temps.
// AUCUNE lecture hors limites, AUCUN secret réel.

#include <stdint.h>
#include <stdio.h>

#define PROBE_SIZE      256
#define STRIDE          64      // espace entre entrées pour séparer les lignes de cache
#define TRAINING_ROUNDS 10000

// Tableau de "probe" utilisé pour encoder la valeur
static uint8_t probe[PROBE_SIZE * STRIDE];

// Petite variable pour éviter que le compilateur n'optimise trop
static volatile uint8_t dummy = 0;

// On simule un "secret" connu, purement pédagogique
static const uint8_t SECRET_VALUE = 42;

// Lire le compteur de cycles RISC-V
static inline uint64_t rdcycle(void) {
    uint64_t x;
    asm volatile ("rdcycle %0" : "=r"(x));
    return x;
}

// Fonction victime : si cond est vrai, elle touche probe[value * STRIDE]
static void victim_function(int cond, uint8_t value) {
    if (cond) {
        // Chemin "pris" : encode value dans le cache
        probe[(size_t)value * STRIDE] += 1;
    } else {
        // Chemin "non pris"
        dummy++;
    }
}

// Petite fonction pour "bruiter" le cache (pas un vrai flush, mais ça aide)
static void trash_cache(void) {
    volatile uint8_t *p = probe;
    for (int i = 0; i < PROBE_SIZE * STRIDE; i += 16) {
        dummy ^= p[i];
    }
}

int main(void) {
    printf("=== Spectre-like RISC-V benchmark (educatif) ===\n");

    // Initialiser probe
    for (int i = 0; i < PROBE_SIZE * STRIDE; i++) {
        probe[i] = 1;
    }

    // 1) ENTRAINEMENT du prédicteur : cond = 1 avec une valeur "banale"
    uint8_t training_value = 10;
    for (int i = 0; i < TRAINING_ROUNDS; i++) {
        victim_function(1, training_value);
    }

    // Petit bruit dans le cache
    trash_cache();

    // 2) PHASE "Spectre-like" :
    //    cond = 0, mais prédicteur potentiellement biaisé vers cond = 1
    //    On encode SECRET_VALUE dans le cache
    victim_function(0, SECRET_VALUE);

    // 3) MESURE DES TEMPS D'ACCES
    uint64_t times[PROBE_SIZE];

    for (int i = 0; i < PROBE_SIZE; i++) {
        uint64_t t1 = rdcycle();
        dummy ^= probe[(size_t)i * STRIDE];
        uint64_t t2 = rdcycle();
        times[i] = t2 - t1;
    }

    // 4) On cherche l'indice le plus rapide
    uint64_t best_time = (uint64_t)-1;
    int best_index = -1;

    for (int i = 0; i < PROBE_SIZE; i++) {
        if (times[i] < best_time) {
            best_time = times[i];
            best_index = i;
        }
    }

    printf("Valeur SECRET_VALUE (connue)  : %d\n", SECRET_VALUE);
    printf("Indice le plus rapide mesuré : %d (temps = %llu cycles)\n",
           best_index, (unsigned long long)best_time);

    if (best_index == SECRET_VALUE) {
        printf(">> On voit un effet cache compatible avec un comportement Spectre-like.\n");
    } else {
        printf(">> Le signal n'est pas très clair (bruit, config cache, etc.).\n");
    }

    // 5) Sauvegarder tous les temps d’accès dans un CSV pour analyse hors-ligne
    FILE *f = fopen("access_times.csv", "w");
    if (!f) {
        perror("fopen access_times.csv");
        return 1;
    }

    // En-tête CSV
    fprintf(f, "index,time_cycles\n");
    for (int i = 0; i < PROBE_SIZE; i++) {
        fprintf(f, "%d,%llu\n", i, (unsigned long long)times[i]);
    }
    fclose(f);

    return 0;
}
