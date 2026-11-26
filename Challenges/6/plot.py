import csv
import matplotlib.pyplot as plt

# Lecture des deux fichiers produits par ./bench
with open("resultats_fermat.csv") as f:
    fermat = list(csv.DictReader(f))
with open("resultats_shor.csv") as f:
    shor = list(csv.DictReader(f))

for row in fermat:
    for k in ("N", "p", "q", "iterations"):
        row[k] = int(row[k])
    row["time_s"] = float(row["time_s"])

for row in shor:
    for k in ("exp", "N", "a", "r", "iterations"):
        row[k] = int(row[k])
    row["time_s"] = float(row["time_s"])

exp1 = sorted((r for r in shor if r["exp"] == 1), key=lambda r: r["N"])
exp2 = [r for r in shor if r["exp"] == 2]
COULEURS = {"OK": "tab:green", "TRIVIAL": "tab:orange",
            "ODD_PERIOD": "tab:red", "NOT_FOUND": "tab:gray", "FAIL": "tab:purple"}


def nouvelle_figure(titre, xlabel, ylabel, logx=False, logy=False):
    fig, ax = plt.subplots(figsize=(9, 5))
    ax.set_title(titre)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if logx:
        ax.set_xscale("log")
    if logy:
        ax.set_yscale("log")
    ax.grid(True, alpha=0.3)
    return fig, ax


def finir(fig, ax, nom, legende=True):
    if legende:
        ax.legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(nom, dpi=150)
    print("Figure enregistrée :", nom)


# 1) Temps en fonction de N : Fermat vs période (a = 2), mêmes N
nf = {r["N"]: r for r in fermat}
communs = [r for r in exp1 if r["N"] in nf]
fig1, ax1 = nouvelle_figure("Temps d'exécution en fonction de N", "N", "Temps (s)",
                            logx=True, logy=True)
ax1.plot([r["N"] for r in communs], [nf[r["N"]]["time_s"] for r in communs],
         marker="o", label="Fermat")
ax1.plot([r["N"] for r in communs], [r["time_s"] for r in communs],
         marker="s", label="Période (a = 2)")
echecs = [r for r in communs if r["status"] != "OK"]
ax1.scatter([r["N"] for r in echecs], [r["time_s"] for r in echecs], marker="x",
            color="red", s=80, zorder=3, label="Période : échec (NOT_FOUND / TRIVIAL)")
finir(fig1, ax1, "temps_vs_N.png")

# 2) Fermat : itérations en fonction du rapport q/p (écart entre facteurs)
ff = sorted(fermat, key=lambda r: r["q"] / r["p"])
fig2, ax2 = nouvelle_figure("Fermat : itérations en fonction de l'écart q/p",
                            "q / p", "Itérations", logx=True, logy=True)
ax2.plot([r["q"] / r["p"] for r in ff], [max(r["iterations"], 1) for r in ff], marker="o")
finir(fig2, ax2, "fermat_iterations_vs_ecart.png", legende=False)

# 3) Période : temps en fonction de r (toutes expériences, r trouvé)
trouves = sorted((r for r in shor if r["r"] > 0), key=lambda r: r["r"])
fig3, ax3 = nouvelle_figure("Période : temps en fonction de l'ordre r", "r", "Temps (s)",
                            logx=True, logy=True)
for st, c in COULEURS.items():
    pts = [r for r in trouves if r["status"] == st]
    if pts:
        ax3.scatter([r["r"] for r in pts], [r["time_s"] for r in pts], color=c, label=st)
finir(fig3, ax3, "shor_temps_vs_r.png")

# 4) Période : r selon la base a (N = 10403)
fig4, ax4 = nouvelle_figure("Influence de la base a (N = 10403)", "Base a", "Période r")
ax4.bar([str(r["a"]) for r in exp2], [r["r"] for r in exp2],
        color=[COULEURS.get(r["status"], "black") for r in exp2])
for i, r in enumerate(exp2):
    ax4.annotate(r["status"], (i, r["r"]), ha="center", fontsize=7,
                 textcoords="offset points", xytext=(0, 3))
finir(fig4, ax4, "shor_bases.png", legende=False)

plt.show()
