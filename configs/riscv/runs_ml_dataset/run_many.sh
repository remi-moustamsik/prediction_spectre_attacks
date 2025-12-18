#!/usr/bin/env bash

# Binaire gem5 + config
GEM5=build/ALL/gem5.opt
CONFIG=configs/learning_gem5/part1/simple-riscv.py
SAFE_CONFIG=configs/learning_gem5/part1/simple-riscv-safe.py

# Dossier racine où tout sera stocké
RESULTS_BASE=results_dataset
mkdir -p "$RESULTS_BASE/spectre" "$RESULTS_BASE/normal"

# Nombre de répétitions par classe
N=5000

for i in $(seq 1 $N); do
    echo "=== [Spectre] run $i / $N ==="
    OUTDIR_ATTACK="$RESULTS_BASE/spectre/run_$i"
    mkdir -p "$OUTDIR_ATTACK"

    --cmd=bench_spectre_like.riscv
    --attack=1
    "$GEM5" \
        --outdir="$OUTDIR_ATTACK" \
        "$CONFIG" \
        --mode=spectre

    echo "=== [Normal]  run $i / $N ==="
    OUTDIR_NORMAL="$RESULTS_BASE/normal/run_$i"
    mkdir -p "$OUTDIR_NORMAL"

       --cmd=bench_normal.riscv
       --attack=0
    "$GEM5" \
        --outdir="$OUTDIR_NORMAL" \
        "$SAFE_CONFIG" \
        --mode=normal
done
