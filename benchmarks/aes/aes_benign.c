#include <stdio.h>
#include "aes.h" // Assuming you downloaded tiny-AES-c
#include <stdint.h>

void run_aes_workload() {
    uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t in[16]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
    struct AES_ctx ctx;

    AES_init_ctx(&ctx, key);
    // Encrypt multiple times to generate CPU/Cache activity
    for(int i=0; i<50; i++) {
        AES_ECB_encrypt(&ctx, in);
    }
}

int main() {
    printf("Starting Benign AES...\n");
    // Run enough times to generate a gem5 trace (e.g., 1000 iterations)
    for (int i = 0; i < 1000; i++) {
        run_aes_workload();
    }
    printf("Done.\n");
    return 0;
}
