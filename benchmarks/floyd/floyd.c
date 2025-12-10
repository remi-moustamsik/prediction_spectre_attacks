#include <stdio.h>
#include <stdlib.h> // Required for qsort

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

int main() {
    printf("Starting Benign Floyd...\n");
    // Run enough times to generate a gem5 trace (e.g., 1000 iterations)
    for (int i = 0; i < 1000; i++) {
        run_floyd_workload();
    }
    printf("Done.\n");
    return 0;
}
