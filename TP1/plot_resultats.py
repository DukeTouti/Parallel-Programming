#!/usr/bin/env python3
"""
Lit resultats_somme.csv (genere par l'exercice 5) et trace :
  1. temps d'execution parallele vs taille du tableau, une courbe par nb_processus
  2. acceleration vs nb_processus, une courbe par taille de tableau
  3. temps d'execution parallele vs nb_processus, une courbe par taille de tableau
Les graphes sont enregistres dans le repertoire screens/.
"""

import csv
import os
from collections import defaultdict

import matplotlib.pyplot as plt

CSV_PATH = "resultats_somme.csv"
OUT_DIR = "screens"


def charger_resultats(chemin):
	lignes = []
	with open(chemin, newline="") as f:
		lecteur = csv.DictReader(f)
		for ligne in lecteur:
			lignes.append({
				"taille_tableau": int(ligne["taille_tableau"]),
				"nb_processus": int(ligne["nb_processus"]),
				"temps_sequentiel": float(ligne["temps_sequentiel"]),
				"temps_parallele": float(ligne["temps_parallele"]),
				"acceleration": float(ligne["acceleration"]),
			})
	return lignes


def tracer_temps_vs_taille(lignes, out_dir):
	par_nprocs = defaultdict(list)
	for l in lignes:
		par_nprocs[l["nb_processus"]].append((l["taille_tableau"], l["temps_parallele"]))

	plt.figure(figsize=(8, 6))
	for nprocs in sorted(par_nprocs):
		points = sorted(par_nprocs[nprocs])
		x = [p[0] for p in points]
		y = [p[1] for p in points]
		plt.plot(x, y, marker="o", label=f"{nprocs} processus")

	plt.xlabel("Taille du tableau (nombre d'entiers)")
	plt.ylabel("Temps d'execution parallele (s)")
	plt.title("Temps d'execution en fonction de la taille du tableau")
	plt.xscale("log")
	plt.yscale("log")
	plt.legend()
	plt.grid(True, which="both", linestyle="--", alpha=0.5)
	plt.tight_layout()
	chemin = os.path.join(out_dir, "temps_vs_taille.png")
	plt.savefig(chemin, dpi=150)
	plt.close()
	print(f"Enregistre : {chemin}")


def tracer_acceleration_vs_nprocs(lignes, out_dir):
	par_taille = defaultdict(list)
	for l in lignes:
		par_taille[l["taille_tableau"]].append((l["nb_processus"], l["acceleration"]))

	plt.figure(figsize=(8, 6))
	for n in sorted(par_taille):
		points = sorted(par_taille[n])
		x = [p[0] for p in points]
		y = [p[1] for p in points]
		plt.plot(x, y, marker="o", label=f"n = {n}")

	if par_taille:
		nprocs_max = max(l["nb_processus"] for l in lignes)
		plt.plot([1, nprocs_max], [1, nprocs_max], linestyle="--", color="gray", label="acceleration ideale")

	plt.xlabel("Nombre de processus MPI")
	plt.ylabel("Acceleration (temps sequentiel / temps parallele)")
	plt.title("Acceleration en fonction du nombre de processus")
	plt.legend()
	plt.grid(True, linestyle="--", alpha=0.5)
	plt.tight_layout()
	chemin = os.path.join(out_dir, "acceleration_vs_nprocs.png")
	plt.savefig(chemin, dpi=150)
	plt.close()
	print(f"Enregistre : {chemin}")


def tracer_temps_vs_nprocs(lignes, out_dir):
	par_taille = defaultdict(list)
	for l in lignes:
		par_taille[l["taille_tableau"]].append((l["nb_processus"], l["temps_parallele"]))

	plt.figure(figsize=(8, 6))
	for n in sorted(par_taille):
		points = sorted(par_taille[n])
		x = [p[0] for p in points]
		y = [p[1] for p in points]
		plt.plot(x, y, marker="o", label=f"n = {n}")

	plt.xlabel("Nombre de processus MPI")
	plt.ylabel("Temps d'execution parallele (s)")
	plt.title("Temps d'execution en fonction du nombre de processus")
	plt.yscale("log")
	plt.legend()
	plt.grid(True, which="both", linestyle="--", alpha=0.5)
	plt.tight_layout()
	chemin = os.path.join(out_dir, "temps_vs_nprocs.png")
	plt.savefig(chemin, dpi=150)
	plt.close()
	print(f"Enregistre : {chemin}")


def main():
	if not os.path.exists(CSV_PATH):
		print(f"Fichier {CSV_PATH} introuvable. Lance d'abord ./run_benchmark.sh (ou make bench).")
		return

	os.makedirs(OUT_DIR, exist_ok=True)

	lignes = charger_resultats(CSV_PATH)
	if not lignes:
		print(f"{CSV_PATH} est vide.")
		return

	tracer_temps_vs_taille(lignes, OUT_DIR)
	tracer_acceleration_vs_nprocs(lignes, OUT_DIR)
	tracer_temps_vs_nprocs(lignes, OUT_DIR)


if __name__ == "__main__":
	main()
