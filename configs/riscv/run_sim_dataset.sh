#!/bin/bash
set -euo pipefail

##############################################
# PARAMÈTRES À ADAPTER
##############################################

# Binaire gem5 (dans ton conteneur, c'est souvent juste "gem5")
GEM5_BIN="gem5"

# Script de config pour les runs SPECTRE-LIKE
# -> doit utiliser ton bench_spectre_like.riscv
CONF_SPECTRE="configs/riscv/riscv_o3_spectre.py"

# Script de config pour les runs NORMAUX
# -> doit lancer un programme RISC-V "innocent" (ex: hello, ou autre bench)
CONF_NORMAL="configs/riscv/riscv_o3_normal.py"

# Nombre de runs de chaque type
N_SPECTRE=10
N_NORMAL=10

# Dossier racine où seront stockés tous les résultats
OUT_ROOT="runs_ml_dataset"

# (Optionnel) script Python d'extraction de features (si tu l'as déjà écrit)
FEATURE_SCRIPT="configs/riscv/extract_features.py"
USE_FEATURE_EXTRACTION=false   # passe à true si tu veux l'appeler


##############################################
# FONCTIONS
##############################################

run_one_sim() {
    local label="$1"      # "spectre" ou "normal"
    local idx="$2"        # numéro du run (01, 02, ...)
    local conf="$3"       # script de config gem5

    # Dossier de sortie, ex: runs_ml_dataset/spectre_01
    local run_dir="${OUT_ROOT}/${label}_$(printf '%02d' "$idx")"

    echo
    echo ">>> Lancement run ${label} #${idx} -> ${run_dir}"

    mkdir -p "$run_dir"

    # Lancer gem5 avec -d pour envoyer m5out dans ce dossier
    # On se place dans la racine du repo pour que les chemins relatifs du conf marchent bien
    (
      cd "$(dirname "$0")"  # se placer là où se trouve le script bash (optionnel mais pratique)
      "$GEM5_BIN" -d "$run_dir" "$conf"
    )

    echo ">>> Simulation terminée pour ${label} #${idx}"

    # Si le bench Spectre-like écrit access_times.csv dans le répertoire courant du conf,
    # on peut le déplacer dans le run_dir (à adapter selon ta config).
    if [ -f "access_times.csv" ]; then
        mv "access_times.csv" "${run_dir}/access_times.csv"
        echo "    -> access_times.csv déplacé dans ${run_dir}"
    fi

    # Optionnel : extraction de features tout de suite après chaque run
    if $USE_FEATURE_EXTRACTION && [ -f "$FEATURE_SCRIPT" ]; then
        echo "    -> Extraction des features avec label ${label}"
        # On mappe le label texte en 0/1 pour le ML
        local y=0
        if [ "$label" = "spectre" ]; then
            y=1
        fi

        python3 "$FEATURE_SCRIPT" \
            --stats "${run_dir}/stats.txt" \
            --access "${run_dir}/access_times.csv" \
            --label "$y" \
            --append-to "dataset.csv"
    fi
}

##############################################
# MAIN
##############################################

echo "=== Lancement de la génération de runs pour dataset ML ==="
echo "Dossier racine des runs : $OUT_ROOT"
mkdir -p "$OUT_ROOT"

echo
echo "=== Runs SPECTRE-LIKE ($N_SPECTRE) ==="
for ((i=1; i<=N_SPECTRE; i++)); do
    run_one_sim "spectre" "$i" "$CONF_SPECTRE"
done

echo
echo "=== Runs NORMAUX ($N_NORMAL) ==="
for ((i=1; i<=N_NORMAL; i++)); do
    run_one_sim "normal" "$i" "$CONF_NORMAL"
done

echo
echo "=== Terminé. Tous les runs sont dans : $OUT_ROOT ==="
echo "Chaque sous-dossier contient au moins stats.txt et config.ini (et access_times.csv pour les runs Spectre-like)."

## COMMANDES UTILES POUR LANCER MANUELLEMENT UNE SIMULATION :
# gem5 -d <run_dir> <config_file.py>
