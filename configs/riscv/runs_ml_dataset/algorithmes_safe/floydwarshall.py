import math
from typing import List


def floyd_warshall(dist_matrix: List[List[float]]) -> List[List[float]]:
    n = len(dist_matrix)
    dist = [row[:] for row in dist_matrix]

    for k in range(n):
        for i in range(n):
            for j in range(n):
                via_k = dist[i][k] + dist[k][j]
                if via_k < dist[i][j]:
                    dist[i][j] = via_k

    return dist


def run_floydwarshall():
    print("=== Floyd–Warshall demo ===")
    inf = math.inf
    dist_matrix = [
        [0, 3, inf, 7],
        [8, 0, 2, inf],
        [5, inf, 0, 1],
        [2, inf, inf, 0],
    ]
    print("Matrice de départ :")
    for row in dist_matrix:
        print(row)

    result = floyd_warshall(dist_matrix)

    print("\nPlus courts chemins :")
    for row in result:
        print(row)


if __name__ == "__main__":
    run_floydwarshall()
