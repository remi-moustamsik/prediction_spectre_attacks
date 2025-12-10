#include <stdio.h>
#include <stdlib.h> // Required for qsort
#include <stdint.h>

uint32_t crc32_for_byte(uint32_t r) {
    for (int j = 0; j < 8; ++j)
        r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

void run_crc_workload() {
    char *data = "123456789";
    uint32_t crc = 0;
    for (size_t i = 0; i < 9; ++i) {
        crc = crc32_for_byte((uint32_t)data[i] ^ crc);
    }
    volatile uint32_t sink = crc; // Prevent optimization
    (void)sink;
}

int main() {
    printf("Starting Benign CRC...\n");
    // Run enough times to generate a gem5 trace (e.g., 1000 iterations)
    for (int i = 0; i < 1000; i++) {
        run_crc_workload();
    }
    printf("Done.\n");
    return 0;
}
