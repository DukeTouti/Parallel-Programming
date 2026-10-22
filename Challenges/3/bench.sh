#!/bin/bash
# Benchmark de rsa_text_mpi : plusieurs tailles de texte x plusieurs nombres de processus.
# Écrit les résultats dans resultats.csv (temps en ms, médiane sur REPS passes).
#
# Usage :
#   bash bench.sh
#   CHARS="100000 1000000" REPS=30 NPLIST="1 2 4 8" bash bench.sh
#   MODE=alea bash bench.sh

EXEC=./rsa_text_mpi
MPIRUN=${MPIRUN:-"mpirun --oversubscribe"}
CHARS=${CHARS:-"100000 1000000 5000000"}
REPS=${REPS:-20}
NPLIST=${NPLIST:-"1 2 4 8"}
MODE=${MODE:-std}
OUT=resultats.csv

if [ ! -x "$EXEC" ]; then
	echo "Erreur : $EXEC introuvable (lance 'make' d'abord)" >&2
	exit 1
fi

CORES=$(nproc 2>/dev/null || echo "?")
echo "Coeurs disponibles : $CORES | tailles : $CHARS | processus : $NPLIST | repetitions : $REPS | mode : $MODE"

for np in $NPLIST; do
	if [ "$CORES" != "?" ] && [ "$np" -gt "$CORES" ]; then
		echo "Attention : np=$np > $CORES coeurs, les temps seront biaises (oversubscribe)" >&2
	fi
done

echo "np,n_chars,mode,reps,enc_total_ms,enc_calc_ms,dec_total_ms,dec_calc_ms" > "$OUT"

for n in $CHARS; do
	for np in $NPLIST; do
		echo "  n=$n np=$np ..." >&2
		ligne=$($MPIRUN -np "$np" "$EXEC" "@$n" "$MODE" "$REPS" csv) || {
			echo "Erreur lors de l'execution (n=$n, np=$np)" >&2
			exit 1
		}
		echo "$ligne" >> "$OUT"
	done
done

echo "Resultats ecrits dans $OUT"
