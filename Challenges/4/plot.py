import csv
import sys
import matplotlib.pyplot as plt

EPS = 1e-9

fichier = sys.argv[1] if len(sys.argv) > 1 else "resultats.csv"

# donnees[(test, tpb, blocks)] = {cpu_s, gpu_s, speedup}
donnees = {}
N_de    = {}

with open(fichier) as f:
    for row in csv.DictReader(f):
        t   = int(row["test"])
        tpb = int(row["tpb"])
        blk = int(row["blocks"])
        N_de[t] = int(row["N"])
        donnees[(t, tpb, blk)] = {
            "cpu": float(row["cpu_s"]),
            "gpu": float(row["gpu_s"]),
            "sp":  float(row["speedup"]),
        }

tests  = sorted(N_de)
Ns     = [N_de[t] for t in tests]
configs = sorted({(tpb, blk) for (_, tpb, blk) in donnees})


def nouvelle_figure(titre, xlabel, ylabel, logx=False):
    fig, ax = plt.subplots(figsize=(9, 5))
    ax.set_title(titre)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if logx:
        ax.set_xscale("log")
    ax.grid(True, alpha=0.3)
    return fig, ax


def finir(fig, ax, nom):
    ax.legend(fontsize=7)
    fig.tight_layout()
    fig.savefig(nom, dpi=150)
    print("Figure enregistrée :", nom)


# 1) Temps GPU en fonction de N, une courbe par config
fig1, ax1 = nouvelle_figure("Temps GPU en fonction de N", "N", "Temps GPU (s)", logx=True)
for (tpb, blk) in configs:
    vals = [donnees.get((t, tpb, blk), {}).get("gpu", None) for t in tests]
    if any(v is not None for v in vals):
        ax1.plot(Ns, vals, marker="o", label=f"tpb={tpb} blk={blk}")
finir(fig1, ax1, "temps_vs_N.png")

# 2) Speedup en fonction de N, une courbe par config
fig2, ax2 = nouvelle_figure("Speedup CPU/GPU en fonction de N", "N", "Speedup", logx=True)
for (tpb, blk) in configs:
    vals = [donnees.get((t, tpb, blk), {}).get("sp", None) for t in tests]
    if any(v is not None for v in vals):
        ax2.plot(Ns, vals, marker="o", label=f"tpb={tpb} blk={blk}")
finir(fig2, ax2, "speedup_vs_N.png")

# 3) Speedup moyen par config (bar chart)
fig3, ax3 = nouvelle_figure("Speedup moyen par configuration", "Configuration", "Speedup moyen")
labels = [f"tpb={tpb}\nblk={blk}" for (tpb, blk) in configs]
moyennes = [
    sum(donnees.get((t, tpb, blk), {}).get("sp", 0) for t in tests) / len(tests)
    for (tpb, blk) in configs
]
ax3.bar(labels, moyennes)
ax3.set_ylabel("Speedup moyen")
fig3.tight_layout()
fig3.savefig("speedup_vs_config.png", dpi=150)
print("Figure enregistrée : speedup_vs_config.png")

# Tableau récapitulatif terminal
print(f"\n{'Test':>4} {'N':>25} " + " ".join(f"{'sp('+str(tpb)+','+str(blk)+')':>12}" for (tpb, blk) in configs))
for t in tests:
    ligne = f"{t:>4} {N_de[t]:>25} "
    ligne += " ".join(f"{donnees.get((t,tpb,blk),{}).get('sp',0):>12.2f}" for (tpb, blk) in configs)
    print(ligne)

plt.show()
