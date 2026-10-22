#!/usr/bin/env python3
"""Graphes à partir de resultats.csv (généré par bench.sh) -> dossier graphes/"""

import csv
import os
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

FICHIER = "resultats.csv"
DOSSIER = "graphes"


def lire():
    if not os.path.exists(FICHIER):
        sys.exit(f"{FICHIER} introuvable : lance d'abord 'make bench'")
    data = defaultdict(dict)  # data[n][np] = {metrique: valeur}
    with open(FICHIER, newline="") as f:
        for ligne in csv.DictReader(f):
            n = int(ligne["n_chars"])
            np_ = int(ligne["np"])
            data[n][np_] = {
                "enc_total": float(ligne["enc_total_ms"]),
                "enc_calc": float(ligne["enc_calc_ms"]),
                "dec_total": float(ligne["dec_total_ms"]),
                "dec_calc": float(ligne["dec_calc_ms"]),
            }
    if not data:
        sys.exit(f"{FICHIER} est vide")
    return data


def fig_temps(data):
    tailles = sorted(data)
    fig, axes = plt.subplots(1, len(tailles), figsize=(5 * len(tailles), 4), squeeze=False)
    for ax, n in zip(axes[0], tailles):
        nps = sorted(data[n])
        ax.plot(nps, [data[n][p]["enc_total"] for p in nps], "o-", label="chiffrement (total)")
        ax.plot(nps, [data[n][p]["dec_total"] for p in nps], "s-", label="déchiffrement (total)")
        ax.plot(nps, [data[n][p]["enc_calc"] for p in nps], "o--", label="chiffrement (calcul)")
        ax.plot(nps, [data[n][p]["dec_calc"] for p in nps], "s--", label="déchiffrement (calcul)")
        ax.set_title(f"{n:,} caractères".replace(",", " "))
        ax.set_xlabel("Nombre de processus")
        ax.set_ylabel("Temps (ms)")
        ax.set_xticks(nps)
        ax.grid(True, alpha=0.3)
    axes[0][0].legend(fontsize=8)
    fig.suptitle("Temps d'exécution selon le nombre de processus")
    fig.tight_layout()
    fig.savefig(os.path.join(DOSSIER, "temps.png"), dpi=150)
    plt.close(fig)


def fig_acceleration(data):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4))
    tous_nps = sorted({p for n in data for p in data[n]})
    for n in sorted(data):
        nps = sorted(data[n])
        ref = nps[0]  # idéalement np = 1
        tot = {p: data[n][p]["enc_total"] + data[n][p]["dec_total"] for p in nps}
        calc = {p: data[n][p]["enc_calc"] + data[n][p]["dec_calc"] for p in nps}
        label = f"{n:,} car.".replace(",", " ")
        s_tot = [tot[ref] / tot[p] * ref for p in nps]
        s_calc = [calc[ref] / calc[p] * ref for p in nps]
        ligne, = ax1.plot(nps, s_tot, "o-", label=f"{label} (avec communication)")
        ax1.plot(nps, s_calc, "o--", color=ligne.get_color(), alpha=0.6, label=f"{label} (calcul seul)")
        ax2.plot(nps, [s / p for s, p in zip(s_tot, nps)], "o-", color=ligne.get_color(), label=label)
    ax1.plot(tous_nps, tous_nps, "k:", label="idéal")
    ax1.set_title("Accélération")
    ax1.set_xlabel("Nombre de processus")
    ax1.set_ylabel("Accélération")
    ax1.set_xticks(tous_nps)
    ax1.grid(True, alpha=0.3)
    ax1.legend(fontsize=7)
    ax2.axhline(1.0, color="k", linestyle=":")
    ax2.set_title("Efficacité (accélération / processus)")
    ax2.set_xlabel("Nombre de processus")
    ax2.set_ylabel("Efficacité")
    ax2.set_xticks(tous_nps)
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(os.path.join(DOSSIER, "acceleration.png"), dpi=150)
    plt.close(fig)


def fig_communication(data):
    tailles = sorted(data)
    fig, axes = plt.subplots(1, len(tailles), figsize=(5 * len(tailles), 4), squeeze=False)
    for ax, n in zip(axes[0], tailles):
        nps = sorted(data[n])
        calc = [data[n][p]["enc_calc"] + data[n][p]["dec_calc"] for p in nps]
        tot = [data[n][p]["enc_total"] + data[n][p]["dec_total"] for p in nps]
        comm = [max(t - c, 0) for t, c in zip(tot, calc)]
        x = range(len(nps))
        ax.bar(x, calc, label="calcul (max processus)")
        ax.bar(x, comm, bottom=calc, label="communication (~)")
        ax.set_xticks(list(x))
        ax.set_xticklabels(nps)
        ax.set_title(f"{n:,} caractères".replace(",", " "))
        ax.set_xlabel("Nombre de processus")
        ax.set_ylabel("Temps chiffrement + déchiffrement (ms)")
        ax.grid(True, axis="y", alpha=0.3)
    axes[0][0].legend(fontsize=8)
    fig.suptitle("Répartition calcul / communication")
    fig.tight_layout()
    fig.savefig(os.path.join(DOSSIER, "communication.png"), dpi=150)
    plt.close(fig)


def main():
    data = lire()
    os.makedirs(DOSSIER, exist_ok=True)
    fig_temps(data)
    fig_acceleration(data)
    fig_communication(data)
    print(f"Graphes ecrits dans {DOSSIER}/ : temps.png, acceleration.png, communication.png")


if __name__ == "__main__":
    main()
