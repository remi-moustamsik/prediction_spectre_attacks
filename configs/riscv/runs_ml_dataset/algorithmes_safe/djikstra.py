import heapq
import math
from typing import (
    Dict,
    List,
    Tuple,
)

Graph = Dict[int, List[Tuple[int, float]]]


def dijkstra(graph: Graph, source: int) -> Dict[int, float]:
    dist = {v: math.inf for v in graph}
    dist[source] = 0.0
    pq = [(0.0, source)]

    while pq:
        d, u = heapq.heappop(pq)
        if d > dist[u]:
            continue

        for v, w in graph[u]:
            nd = d + w
            if nd < dist.get(v, math.inf):
                dist[v] = nd
                heapq.heappush(pq, (nd, v))

    return dist


def run_dijkstra():
    print("=== Dijkstra demo ===")
    graph: Graph = {
        0: [(1, 1.0), (2, 4.0)],
        1: [(2, 2.0), (3, 5.0)],
        2: [(3, 1.0)],
        3: [],
    }
    source = 0
    distances = dijkstra(graph, source)
    print(f"Graph : {graph}")
    print(f"Distances depuis {source} : {distances}")


if __name__ == "__main__":
    run_dijkstra()
