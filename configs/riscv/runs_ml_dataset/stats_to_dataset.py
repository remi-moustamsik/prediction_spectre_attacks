#!/usr/bin/env python3
import csv
from pathlib import Path

# Dossier où le script Bash a mis les résultats
RESULTS_BASE = Path("results_dataset")

# Stats que tu veux extraire de chaque stats.txt.
# ADAPTE cette liste à ce qui t'intéresse pour ton IA.
STATS_OF_INTEREST = [
    "simTicks",
    "simSeconds",
    "simInsts",
    "simOps",
    "system.cpu.icache.overall_miss_rate::total",
    "system.cpu.dcache.overall_miss_rate::total",
]


def parse_stats_file(stats_path):
    """
    Lit un stats.txt gem5 et renvoie un dict {stat_name: value}
    uniquement pour STATS_OF_INTEREST.
    """
    stats = {}
    if not stats_path.is_file():
        print(f"[WARN] Fichier stats manquant : {stats_path}")
        return stats

    with stats_path.open() as f:
        for line in f:
            line = line.strip()
            # ignorer les commentaires et lignes vides
            if not line or line.startswith("#") or line.startswith("-"):
                continue

            parts = line.split()
            if len(parts) < 2:
                continue

            name = parts[0]
            value = parts[1]
            if name in STATS_OF_INTEREST:
                stats[name] = value

    return stats


def main():
    classes = [
        ("spectre", 1),
        ("normal", 0),
    ]

    csv_path = RESULTS_BASE / "dataset.csv"

    fieldnames = ["class_name", "class_label", "run_id"] + STATS_OF_INTEREST

    with csv_path.open("w", newline="") as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for class_name, class_label in classes:
            class_dir = RESULTS_BASE / class_name
            if not class_dir.is_dir():
                print(
                    f"[WARN] Dossier manquant pour la classe {class_name}: {class_dir}"
                )
                continue

            # on s'attend à des sous-dossiers run_1, run_2, ...
            runs = sorted(d for d in class_dir.iterdir() if d.is_dir())

            for run_dir in runs:
                stats_file = run_dir / "stats.txt"
                stats = parse_stats_file(stats_file)

                # run_id = partie après 'run_'
                try:
                    run_id = int(run_dir.name.split("_")[-1])
                except ValueError:
                    run_id = run_dir.name  # fallback

                row = {
                    "class_name": class_name,
                    "class_label": class_label,
                    "run_id": run_id,
                }
                for stat in STATS_OF_INTEREST:
                    row[stat] = stats.get(stat, "")

                writer.writerow(row)
                print(f"[OK] {class_name} run {run_id} ajouté au dataset")

    print(f"\nDataset CSV généré : {csv_path}")


if __name__ == "__main__":
    main()
