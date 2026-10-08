import csv
import sys

import matplotlib.pyplot as plt

EPS = 1e-9  # évite une division par zéro si un temps mesuré vaut 0

fichier = sys.argv[1] if len(sys.argv) > 1 else "resultats.csv"

# temps[(test, np)] = secondes ; N_de[test] = N
temps = {}
N_de = {}
with open(fichier) as f:
    for row in csv.DictReader(f):
        t, n, p = int(row["test"]), int(row["N"]), int(row["np"])
        temps[(t, p)] = float(row["temps_s"])
        N_de[t] = n

tests = sorted(N_de)
nps = sorted({p for (_, p) in temps})
Ns = [N_de[t] for t in tests]


def speedup(t, p):
    return temps[(t, 1)] / max(temps[(t, p)], EPS)


def nouvelle_figure(titre, xlabel, ylabel, logx=False):
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.set_title(titre)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if logx:
        ax.set_xscale("log")
    ax.grid(True, alpha=0.3)
    return fig, ax


def finir(fig, ax, nom):
    ax.legend()
    fig.tight_layout()
    fig.savefig(nom, dpi=150)
    print("Figure enregistrée :", nom)


# 1) Temps en fonction de N, une courbe par nombre de processus
fig1, ax1 = nouvelle_figure("Temps d'exécution en fonction de N", "N", "Temps (s)", logx=True)
for p in nps:
    ax1.plot(Ns, [temps[(t, p)] for t in tests], marker="o", label=f"{p} proc")
finir(fig1, ax1, "temps_vs_N.png")

# 2) Speedup en fonction de N (T1 / Tp)
fig2, ax2 = nouvelle_figure("Speedup en fonction de N", "N", "Speedup  T1 / Tp", logx=True)
for p in nps:
    if p != 1:
        ax2.plot(Ns, [speedup(t, p) for t in tests], marker="o", label=f"{p} proc")
finir(fig2, ax2, "speedup_vs_N.png")

# 3) Speedup moyen en fonction du nombre de processus, avec l'idéal
fig3, ax3 = nouvelle_figure("Speedup en fonction du nombre de processus",
                            "Nombre de processus", "Speedup")
moy = [sum(speedup(t, p) for t in tests) / len(tests) for p in nps]
ax3.plot(nps, moy, marker="o", label="Speedup mesuré (moyenne)")
ax3.plot(nps, nps, "--", color="gray", label="Speedup idéal")
finir(fig3, ax3, "speedup_vs_np.png")

# Tableau récapitulatif dans le terminal
print(f"\n{'Test':>4} {'N':>22} " + " ".join(f"{'T('+str(p)+')':>9}" for p in nps)
      + " " + " ".join(f"{'S('+str(p)+')':>7}" for p in nps if p != 1))
for t in tests:
    ligne = f"{t:>4} {N_de[t]:>22} "
    ligne += " ".join(f"{temps[(t, p)]:>9.4f}" for p in nps) + " "
    ligne += " ".join(f"{speedup(t, p):>7.2f}" for p in nps if p != 1)
    print(ligne)

plt.show()
