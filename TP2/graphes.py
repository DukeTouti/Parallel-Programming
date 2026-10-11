#!/usr/bin/env python3

# Genere les graphes du TP2 (temps, acceleration, efficacite, decomposition,
# complexite) a partir du CSV produit par le benchmark.
# Usage : python3 graphes.py resultats.csv graphes
# Dependances : pandas, numpy, matplotlib

import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

TITRES = {
	"vecteur": "Addition de vecteurs",
	"matvec": "Multiplication matrice x vecteur",
	"matmat": "Multiplication matrice x matrice",
}
EXPOSANTS = {"vecteur": 1, "matvec": 2, "matmat": 3}
COMPLEXITES = {"vecteur": "O(n)", "matvec": "O(n²)", "matmat": "O(n³)"}


# ---------- Chargement ----------

def charger(chemin):
	df = pd.read_csv(chemin)
	df["speedup"] = df["t_seq"] / df["t_par"]
	df["efficacite"] = df["speedup"] / df["nb_processus"]
	df["t_comm"] = (df["t_par"] - df["t_calcul"]).clip(lower=0) # communication + synchronisation
	return df


def sauver(fig, dossier, nom):
	chemin = os.path.join(dossier, nom + ".png")
	fig.tight_layout()
	fig.savefig(chemin, dpi=150)
	plt.close(fig)
	print("  ->", chemin)


# ---------- Figures ----------

def figure_temps(moy, seq, op, dossier):
	fig, ax = plt.subplots(figsize=(7, 5))
	ax.plot(seq.index, seq.values, "k--o", label="séquentiel")
	for p, g in moy.groupby("nb_processus"):
		ax.plot(g["taille"], g["t_par"], marker="o", label=f"P = {p}")
	ax.set_xscale("log")
	ax.set_yscale("log")
	ax.set_xlabel("taille n")
	ax.set_ylabel("temps (s)")
	ax.set_title(TITRES[op] + " : temps d'exécution")
	ax.grid(True, which="both", alpha=0.3)
	ax.legend()
	sauver(fig, dossier, op + "_temps")


def figure_acceleration(moy, op, dossier):
	fig, ax = plt.subplots(figsize=(7, 5))
	for n, g in moy.groupby("taille"):
		ax.plot(g["nb_processus"], g["speedup"], marker="o", label=f"n = {n}")
	procs = sorted(moy["nb_processus"].unique())
	ax.plot(procs, procs, "k--", label="idéal")
	ax.set_xlabel("nombre de processus P")
	ax.set_ylabel("accélération S = T_seq / T_par")
	ax.set_title(TITRES[op] + " : accélération")
	ax.set_xticks(procs)
	ax.grid(True, alpha=0.3)
	ax.legend()
	sauver(fig, dossier, op + "_acceleration")


def figure_efficacite(moy, op, dossier):
	fig, ax = plt.subplots(figsize=(7, 5))
	for n, g in moy.groupby("taille"):
		ax.plot(g["nb_processus"], g["efficacite"], marker="o", label=f"n = {n}")
	ax.axhline(1.0, color="k", linestyle="--", label="idéal")
	ax.set_xlabel("nombre de processus P")
	ax.set_ylabel("efficacité E = S / P")
	ax.set_title(TITRES[op] + " : efficacité")
	ax.set_xticks(sorted(moy["nb_processus"].unique()))
	ax.grid(True, alpha=0.3)
	ax.legend()
	sauver(fig, dossier, op + "_efficacite")


def figure_decomposition(moy, op, dossier):
	n_max = moy["taille"].max() # decomposition sur la plus grande taille
	g = moy[moy["taille"] == n_max].sort_values("nb_processus")
	etiquettes = [str(p) for p in g["nb_processus"]]

	fig, ax = plt.subplots(figsize=(7, 5))
	ax.bar(etiquettes, g["t_calcul"], label="calcul (max sur les processus)")
	ax.bar(etiquettes, g["t_comm"], bottom=g["t_calcul"], label="communication + synchronisation")
	ax.set_xlabel("nombre de processus P")
	ax.set_ylabel("temps (s)")
	ax.set_title(f"{TITRES[op]} : calcul vs communication (n = {n_max})")
	ax.grid(True, axis="y", alpha=0.3)
	ax.legend()
	sauver(fig, dossier, op + "_decomposition")


def figure_complexite(seq, op, dossier):
	k = EXPOSANTS[op]
	n = seq.index.values.astype(float)
	t = seq.values
	pente, _ = np.polyfit(np.log(n), np.log(t), 1) # pente log-log = exposant mesure

	fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.5))
	ax1.plot(n, t, "o-", label=f"mesuré (pente = {pente:.2f})")
	ax1.plot(n, t[-1] * (n / n[-1]) ** k, "k--", label=f"référence {COMPLEXITES[op]}")
	ax1.set_xscale("log")
	ax1.set_yscale("log")
	ax1.set_xlabel("taille n")
	ax1.set_ylabel("T_seq (s)")
	ax1.set_title("Complexité séquentielle (log-log)")
	ax1.grid(True, which="both", alpha=0.3)
	ax1.legend()

	ax2.plot(n, t / n ** k, "o-")
	ax2.set_xscale("log")
	ax2.set_xlabel("taille n")
	ax2.set_ylabel(f"T_seq / n^{k}")
	ax2.set_title(f"Normalisation par {COMPLEXITES[op]} (constante si le modèle tient)")
	ax2.grid(True, which="both", alpha=0.3)
	sauver(fig, dossier, op + "_complexite")
	return pente


# ---------- Programme principal ----------

def main():
	chemin = sys.argv[1] if len(sys.argv) > 1 else "resultats.csv"
	dossier = sys.argv[2] if len(sys.argv) > 2 else "graphes"
	os.makedirs(dossier, exist_ok=True)

	df = charger(chemin)
	for op in ["vecteur", "matvec", "matmat"]:
		df_op = df[df["operation"] == op]
		if df_op.empty:
			continue
		print(TITRES[op])

		moy = df_op.groupby(["taille", "nb_processus"], as_index=False).mean(numeric_only=True)
		seq = df_op.groupby("taille")["t_seq"].median() # t_seq est mesure a chaque P, on prend la mediane

		figure_temps(moy, seq, op, dossier)
		figure_acceleration(moy, op, dossier)
		figure_efficacite(moy, op, dossier)
		figure_decomposition(moy, op, dossier)
		pente = figure_complexite(seq, op, dossier)

		print(f"  pente log-log mesurée = {pente:.2f} (théorie : {EXPOSANTS[op]})")
		print("  accélération (lignes : n, colonnes : P)")
		print(moy.pivot(index="taille", columns="nb_processus", values="speedup").round(2).to_string())
		print()


if __name__ == "__main__":
	main()
