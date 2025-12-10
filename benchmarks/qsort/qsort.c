#include <stdio.h>
#include <stdlib.h> // Required for qsort

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

int main() {
    printf("Starting Benign QSort...\n");
    // Run enough times to generate a gem5 trace (e.g., 1000 iterations)
    for (int i = 0; i < 1000; i++) {
        run_qsort_workload();
    }
    printf("Done.\n");
    return 0;
}
