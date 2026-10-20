#!/usr/bin/env python3

# Usage : python3 graphes.py resultats.csv graphes
# Dependances : pandas, matplotlib

import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

LABELS = {
    "N_1":   "<<<N, 1>>>",
    "N_T_T": "<<<N/T, T>>>",
    "1_N":   "<<<1, N>>>",
}


# ---------- Chargement ----------

def charger(chemin):
    df = pd.read_csv(chemin)
    df["speedup"] = df["t_cpu"] / df["t_gpu"]
    df["mflops"]  = df["taille"] / df["t_gpu"] / 1e6
    return df


def sauver(fig, dossier, nom):
    chemin = os.path.join(dossier, nom + ".png")
    fig.tight_layout()
    fig.savefig(chemin, dpi=150)
    plt.close(fig)
    print("  ->", chemin)


# ---------- Figures ----------

def figure_temps(df, dossier):
    fig, ax = plt.subplots(figsize=(8, 5))

    # Reference CPU (on prend la mediane par taille)
    cpu_ref = df.groupby("taille")["t_cpu"].median()
    ax.plot(cpu_ref.index, cpu_ref.values, "k--o", label="CPU (reference)")

    for config, g in df.groupby("config"):
        moy = g.groupby("taille")["t_gpu"].mean()
        ax.plot(moy.index, moy.values, marker="o", label=LABELS.get(config, config))

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("taille N")
    ax.set_ylabel("temps (s)")
    ax.set_title("Addition de vecteurs : temps d'execution")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    sauver(fig, dossier, "vecadd_temps")


def figure_speedup(df, dossier):
    fig, ax = plt.subplots(figsize=(8, 5))

    for config, g in df.groupby("config"):
        moy = g.groupby("taille")["speedup"].mean()
        ax.plot(moy.index, moy.values, marker="o", label=LABELS.get(config, config))

    ax.axhline(1.0, color="k", linestyle="--", label="S = 1")
    ax.set_xscale("log")
    ax.set_xlabel("taille N")
    ax.set_ylabel("acceleration S = T_cpu / T_gpu")
    ax.set_title("Addition de vecteurs : acceleration GPU vs CPU")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    sauver(fig, dossier, "vecadd_speedup")


def figure_mflops(df, dossier):
    fig, ax = plt.subplots(figsize=(8, 5))

    for config, g in df.groupby("config"):
        moy = g.groupby("taille")["mflops"].mean()
        ax.plot(moy.index, moy.values, marker="o", label=LABELS.get(config, config))

    ax.set_xscale("log")
    ax.set_xlabel("taille N")
    ax.set_ylabel("MFLOPS")
    ax.set_title("Addition de vecteurs : debit par configuration")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    sauver(fig, dossier, "vecadd_mflops")


# ---------- Programme principal ----------

def main():
    chemin = sys.argv[1] if len(sys.argv) > 1 else "resultats.csv"
    dossier = sys.argv[2] if len(sys.argv) > 2 else "graphes"
    os.makedirs(dossier, exist_ok=True)

    df = charger(chemin)

    figure_temps(df, dossier)
    figure_speedup(df, dossier)
    figure_mflops(df, dossier)

    print("\nAcceleration par config et taille :")
    pivot = df.groupby(["taille", "config"])["speedup"].mean().unstack()
    pivot.columns = [LABELS.get(c, c) for c in pivot.columns]
    print(pivot.round(2).to_string())


if __name__ == "__main__":
    main()
