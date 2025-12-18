#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define FLUSH_BUF_SIZE (1024 * 1024)   // 1 MiB > L1+L2
#define ARRAY2_STRIDE  512             // comme dans ton code Spectre

// ======================================================================
// GLOBALS
// ======================================================================

uint8_t array2[256 * ARRAY2_STRIDE];
uint8_t flush_buf[FLUSH_BUF_SIZE];

uint8_t temp = 0;

// ======================================================================
// TIMING
// ======================================================================

static inline uint64_t read_cycle(void) {
    uint64_t cycle;
    asm volatile ("rdcycle %0" : "=r"(cycle));
    return cycle;
}

__attribute__((noinline))
uint64_t measure_idx(int k) {
    volatile uint8_t *addr = &array2[k * ARRAY2_STRIDE];

    asm volatile("fence rw,rw" ::: "memory");
    uint64_t start = read_cycle();
    temp &= *addr;
    asm volatile("fence rw,rw" ::: "memory");
    uint64_t end = read_cycle();

    return end - start;
}

// ======================================================================
// VICTIME : branche simple à entraîner
// ======================================================================
//
// On choisit un seuil constant (1) pour que la branche soit :
//   - vraie si x == 0
//   - fausse si x == 1
//
// Le but : entraîner la prédiction à "toujours prise" (x=0),
// puis provoquer un cas x=1 et voir si le load spéculatif chauffe array2[0].
//

__attribute__((noinline))
void victim(size_t x) {
    if (x < 1) {
        // accès qui devrait être exécuté *seulement* si la branche est prise
        temp &= array2[0 * ARRAY2_STRIDE];
    }
}

// ======================================================================
// MAIN
// ======================================================================

int main(void) {
    // ------------------------------------------------------------------
    // 1) Init des buffers
    // ------------------------------------------------------------------
    for (int i = 0; i < (int)sizeof(array2); i++) {
        array2[i] = 1;
    }
    for (size_t i = 0; i < FLUSH_BUF_SIZE; i++) {
        flush_buf[i] = (uint8_t)i;
    }

    const int WARMUP_TRAIN = 10000;   // gros entraînement initial
    const int TRAIN_INNER  = 50;      // small training avant chaque attaque
    const int ATTEMPTS     = 200;     // nombre d’expériences
    const int THRESH       = 10;      // marge pour départager t0 < t1

    printf("=== Speculative load micro-test ===\n");
    printf("Branch: if (x < 1) { load array2[0] }\n");
    printf("Training with x = 0, attack with x = 1.\n\n");

    // ------------------------------------------------------------------
    // 2) GROS ENTRAÎNEMENT GLOBAL : x=0 -> branche toujours prise
    // ------------------------------------------------------------------
    for (int t = 0; t < WARMUP_TRAIN; t++) {
        victim(0);
    }

    // ------------------------------------------------------------------
    // 3) Boucle d’attaques + mesures
    // ------------------------------------------------------------------
    int wins0 = 0;   // nombre de fois où t0 << t1
    int wins1 = 0;   // nombre de fois où t1 << t0 (contrôle)
    int ties  = 0;   // "égalité"

    for (int attempt = 0; attempt < ATTEMPTS; attempt++) {
        // 3.a) Thrash du cache avec flush_buf (PAS array2)
        for (size_t i = 0; i < FLUSH_BUF_SIZE; i += 64) {
            flush_buf[i]++;
        }

        // 3.b) re-training local de la branche : x=0
        for (int t = 0; t < TRAIN_INNER; t++) {
            victim(0);   // branche vraie -> "taken"
        }

        // 3.c) ATTACK : x=1 -> architecturiellement, branche non prise
        // On espère : prédicteur "taken" -> exécution spéculative du load.
        victim(1);

        // 3.d) MESURE : temps d’accès sur array2[0] (potentiellement chauffé)
        //              vs array2[1] (contrôle)
        uint64_t t0 = measure_idx(0);
        uint64_t t1 = measure_idx(1);

        // petit diagnostic sur les premières itérations
        if (attempt < 10) {
            printf("[Attempt %3d] t0=%4lu, t1=%4lu\n",
                   attempt, (unsigned long)t0, (unsigned long)t1);
        }

        if (t0 + THRESH < t1) {
            wins0++;
        } else if (t1 + THRESH < t0) {
            wins1++;
        } else {
            ties++;
        }
    }

    // ------------------------------------------------------------------
    // 4) Résumé des résultats
    // ------------------------------------------------------------------
    printf("\n=== Results over %d attempts ===\n", ATTEMPTS);
    printf("t0 = access to array2[0] (speculative load target)\n");
    printf("t1 = access to array2[1] (control)\n");
    printf("wins0 (t0 << t1) = %d\n", wins0);
    printf("wins1 (t1 << t0) = %d\n", wins1);
    printf("ties              = %d\n", ties);
    printf("temp = %d\n", temp);

    return 0;
}
