#!/bin/bash

# ===============================
# Script pour exécuter analyze_spectre_bench.py
# et enregistrer la sortie dans un fichier texte
# ===============================

# Emplacement du script Python (modifiable)
SCRIPT="/workspaces/gem5/configs/riscv/analyze_spectre.py"

# Vérifier que le fichier existe
if [ ! -f "$SCRIPT" ]; then
    echo "[ERREUR] Script Python introuvable : $SCRIPT"
    exit 1
fi

# Création d’un nom de fichier unique basé sur la date/heure
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")
OUTFILE="analyse_spectre_${TIMESTAMP}.txt"

echo "[INFO] Exécution du script…"
echo "[INFO] La sortie sera enregistrée dans : $OUTFILE"

# Exécution du script Python et capture de toute la sortie
python3 "$SCRIPT" > "$OUTFILE" 2>&1

# Vérification
if [ $? -eq 0 ]; then
    echo "[OK] Analyse terminée. Résultats dans : $OUTFILE"
else
    echo "[ERREUR] Le script Python a rencontré une erreur."
    echo "Consulte : $OUTFILE"
fi
