#!/usr/bin/env python3
"""
Lit resultats.csv (genere par le jeu de test) et trace :
  1. temps d'execution vs taille (sequentiel, MPI pour chaque P, CUDA pour chaque methode)
  2. acceleration vs taille
  3. acceleration MPI vs nombre de processus, une courbe par taille
  4. decomposition du temps (communication, phase 1, phase 2) pour la plus grande taille
Les graphes sont enregistres dans le repertoire graphes/.
"""

import csv
import os
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

CSV_PATH = "resultats.csv"
OUT_DIR = "graphes"


def charger_resultats(chemin):
	lignes = []
	with open(chemin, newline="") as f:
		lecteur = csv.DictReader(f)
		for ligne in lecteur:
			lignes.append({
				"version": ligne["version"],
				"configuration": ligne["configuration"],
				"nom": ligne["version"] + " " + ligne["configuration"],
				"taille": int(ligne["taille"]),
				"nb_processus": int(ligne["nb_processus"]),
				"t_seq": float(ligne["t_seq"]),
				"t_communication": float(ligne["t_communication"]),
				"t_phase1": float(ligne["t_phase1"]),
				"t_phase2": float(ligne["t_phase2"]),
				"t_total": float(ligne["t_total"]),
				"acceleration": float(ligne["acceleration"]),
				"correct": int(ligne["correct"]),
			})
	return lignes


def sauver(nom):
	chemin = os.path.join(OUT_DIR, nom)
	plt.tight_layout()
	plt.savefig(chemin, dpi=150)
	plt.close()
	print(f"Enregistre : {chemin}")


def tracer_temps_vs_taille(lignes):
	par_config = defaultdict(list)
	seq = {}
	for l in lignes:
		par_config[l["nom"]].append((l["taille"], l["t_total"]))
		seq[l["taille"]] = l["t_seq"]

	plt.figure(figsize=(9, 6))
	tailles = sorted(seq)
	plt.plot(tailles, [seq[n] for n in tailles], "k--o", label="sequentiel")
	for nom in par_config:
		points = sorted(par_config[nom])
		plt.plot([p[0] for p in points], [p[1] for p in points], marker="o", label=nom)

	plt.xlabel("Taille n des sequences (matrice n x n)")
	plt.ylabel("Temps d'execution total (s)")
	plt.title("LCS : temps d'execution en fonction de la taille")
	plt.xscale("log")
	plt.yscale("log")
	plt.legend()
	plt.grid(True, which="both", linestyle="--", alpha=0.5)
	sauver("temps_vs_taille.png")


def tracer_acceleration_vs_taille(lignes):
	par_config = defaultdict(list)
	for l in lignes:
		par_config[l["nom"]].append((l["taille"], l["acceleration"]))

	plt.figure(figsize=(9, 6))
	for nom in par_config:
		points = sorted(par_config[nom])
		plt.plot([p[0] for p in points], [p[1] for p in points], marker="o", label=nom)

	plt.axhline(1.0, color="gray", linestyle="--", label="acceleration = 1")
	plt.xlabel("Taille n des sequences")
	plt.ylabel("Acceleration (temps sequentiel / temps parallele)")
	plt.title("LCS : acceleration en fonction de la taille")
	plt.xscale("log")
	plt.yscale("log")
	plt.legend()
	plt.grid(True, which="both", linestyle="--", alpha=0.5)
	sauver("acceleration_vs_taille.png")


def tracer_acceleration_mpi_vs_nprocs(lignes):
	par_taille = defaultdict(list)
	for l in lignes:
		if l["version"] == "MPI" and l["taille"] >= 1000:
			par_taille[l["taille"]].append((l["nb_processus"], l["acceleration"]))
	if not par_taille:
		return

	plt.figure(figsize=(9, 6))
	for n in sorted(par_taille):
		points = sorted(par_taille[n])
		plt.plot([p[0] for p in points], [p[1] for p in points], marker="o", label=f"n = {n}")

	nprocs_max = max(p for n in par_taille for p, _ in par_taille[n])
	plt.plot([1, nprocs_max], [1, nprocs_max], linestyle="--", color="gray", label="acceleration ideale")
	plt.xlabel("Nombre de processus MPI")
	plt.ylabel("Acceleration (temps sequentiel / temps parallele)")
	plt.title("MPI : acceleration en fonction du nombre de processus")
	plt.legend()
	plt.grid(True, linestyle="--", alpha=0.5)
	sauver("acceleration_mpi_vs_nprocs.png")


def tracer_decomposition(lignes):
	n_max = max(l["taille"] for l in lignes)
	points = [l for l in lignes if l["taille"] == n_max]
	noms = [l["nom"] for l in points]
	x = range(len(points))
	communication = [l["t_communication"] for l in points]
	phase1 = [l["t_phase1"] for l in points]
	phase2 = [l["t_phase2"] for l in points]
	bas_phase2 = [c + p for c, p in zip(communication, phase1)]

	plt.figure(figsize=(9, 6))
	plt.bar(x, communication, label="communication (MPI) / transfert (CUDA)")
	plt.bar(x, phase1, bottom=communication, label="phase 1 : matrice des scores")
	plt.bar(x, phase2, bottom=bas_phase2, label="phase 2 : traceback")
	plt.xticks(list(x), noms, rotation=20)
	plt.ylabel("Temps (s)")
	plt.title(f"Decomposition du temps (n = {n_max})")
	plt.legend()
	plt.grid(True, axis="y", linestyle="--", alpha=0.5)
	sauver("decomposition.png")


def main():
	if not os.path.exists(CSV_PATH):
		print(f"Fichier {CSV_PATH} introuvable. Lance d'abord ./jeu_de_test (ou make run).")
		return

	os.makedirs(OUT_DIR, exist_ok=True)

	lignes = charger_resultats(CSV_PATH)
	if not lignes:
		print(f"{CSV_PATH} est vide.")
		return

	tracer_temps_vs_taille(lignes)
	tracer_acceleration_vs_taille(lignes)
	tracer_acceleration_mpi_vs_nprocs(lignes)
	tracer_decomposition(lignes)


if __name__ == "__main__":
	main()
