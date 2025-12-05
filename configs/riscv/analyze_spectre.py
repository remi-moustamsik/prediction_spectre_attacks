#!/usr/bin/env python3
# analyze_spectre_bench.py
#
# Analyse du micro-benchmark Spectre-like :
# - parse m5out/stats.txt pour extraire quelques stats intéressantes
# - parse access_times.csv pour voir quels indices sont les plus rapides

import csv
from pathlib import Path

STATS_PATH = Path("/workspaces/gem5/m5out/stats.txt")
ACCESS_TIMES_PATH = Path("access_times.csv")


from pathlib import Path


def parse_stats(path: Path) -> dict:
    """
    Parse le fichier stats.txt et retourne un dict {stat_name: value}
    """
    stats = {}

    if not path.exists():
        print(f"[!] Fichier stats introuvable : {path}")
        return {}

    with path.open("r") as f:
        for line in f:
            # Ignore les lignes non pertinentes
            if (
                line.strip() == ""
                or line.startswith("-")
                or line.startswith("End")
            ):
                continue

            parts = line.split()
            if len(parts) >= 2:
                key = parts[0]
                val = parts[1]

                # Si val est numérique, convertis-la
                try:
                    if "." in val:
                        val = float(val)
                    else:
                        val = int(val)
                except ValueError:
                    pass

                stats[key] = val

    return stats


def parse_access_times(path: Path):
    """
    Parse access_times.csv (format : index,time_cycles)
    Retourne une liste de tuples (index:int, time:int)
    """
    results = []
    if not path.exists():
        print(f"[!] Fichier des temps d'accès non trouvé : {path}")
        return results

    with path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            idx = int(row["index"])
            t = int(row["time_cycles"])
            results.append((idx, t))

    return results


def summarize_access_times(times):
    if not times:
        print("[!] Aucun temps d'accès à analyser.")
        return

    # Trie par temps croissant (les plus rapides d'abord)
    times_sorted = sorted(times, key=lambda x: x[1])

    best_index, best_time = times_sorted[0]
    print("\n=== Analyse des temps d'accès (access_times.csv) ===")
    print(f"Indice le plus rapide      : {best_index}")
    print(f"Temps le plus bas (cycles) : {best_time}")

    print("\nTop 10 indices les plus rapides :")
    for rank, (i, t) in enumerate(times_sorted[:10], start=1):
        print(f"{rank:2d}. index = {i:3d}, time = {t} cycles")


def summarize_stats(stats: dict):
    if not stats:
        print("[!] Pas de stats gem5 à analyser.")
        return

    print("\n=== Extraits de m5out/stats.txt ===")

    # Noms adaptés à ce que tu as montré dans ton stats.txt
    keys_of_interest = [
        "simSeconds",
        "simTicks",
        "simInsts",
        "simOps",
        "hostSeconds",
        "hostInstRate",
        # Statistiques L1D spécifiques à ta config Components API
        "board.cache_hierarchy.l1d-cache-0.demandMissRate::total",
        "board.cache_hierarchy.l1d-cache-0.overallMissRate::total",
        "board.cache_hierarchy.l1d-cache-0.demandAccesses::total",
        "board.cache_hierarchy.l1d-cache-0.demandMisses::total",
    ]

    for key in keys_of_interest:
        if key in stats:
            print(f"{key:60s} = {stats[key]}")
        else:
            print(f"{key:60s} = (non présent)")

    # Exemple : calculer un IPC si possible
    sim_insts = stats.get("simInsts")
    sim_ticks = stats.get("simTicks")
    if sim_insts is not None and sim_ticks is not None and sim_ticks > 0:
        ipc = (
            sim_insts / sim_ticks
        )  # c'est plutôt "inst per tick", mais ça donne une idée
        print(f"\nInstructions par tick (simInsts/simTicks) : {ipc:.6e}")


def main():
    print("=== Analyse du benchmark Spectre-like (gem5 RISC-V O3) ===")

    stats = parse_stats(STATS_PATH)
    access_times = parse_access_times(ACCESS_TIMES_PATH)

    summarize_stats(stats)
    summarize_access_times(access_times)


if __name__ == "__main__":
    main()
