import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

FICHIER_CSV = "resultats.csv"
DOSSIER = "screens"

# Lecture du CSV : la ligne "seq" est le temps séquentiel T1
sequentiel = None
np_liste = []
temps = []
acceleration = []
efficacite = []

with open(FICHIER_CSV, newline="") as f:
	for ligne in csv.DictReader(f):
		if ligne["np"] == "seq":
			sequentiel = float(ligne["temps_s"])
		else:
			np_liste.append(int(ligne["np"]))
			temps.append(float(ligne["temps_s"]))
			acceleration.append(float(ligne["acceleration"]))
			efficacite.append(float(ligne["efficacite"]))

os.makedirs(DOSSIER, exist_ok=True)


def enregistrer(nom):
	plt.tight_layout()
	plt.savefig(os.path.join(DOSSIER, nom), dpi=150)
	plt.close()


# 1. Temps d'exécution en fonction de P
plt.figure(figsize=(6, 4))
plt.plot(np_liste, temps, "o-", label="MPI")
if sequentiel is not None:
	plt.axhline(sequentiel, color="gray", linestyle="--", label="Séquentiel")
plt.xticks(np_liste)
plt.xlabel("Nombre de processus P")
plt.ylabel("Temps d'exécution (s)")
plt.title("Temps d'exécution en fonction de P")
plt.grid(True, alpha=0.3)
plt.legend()
enregistrer("temps_vs_np.png")

# 2. Accélération en fonction de P (avec l'accélération idéale)
plt.figure(figsize=(6, 4))
plt.plot(np_liste, acceleration, "o-", label="Mesurée")
plt.plot(np_liste, np_liste, "k--", label="Idéale (S = P)")
plt.xticks(np_liste)
plt.xlabel("Nombre de processus P")
plt.ylabel("Accélération S(P)")
plt.title("Accélération en fonction de P")
plt.grid(True, alpha=0.3)
plt.legend()
enregistrer("acceleration_vs_np.png")

# 3. Efficacité en fonction de P
plt.figure(figsize=(6, 4))
plt.plot(np_liste, efficacite, "o-", label="Mesurée")
plt.axhline(1.0, color="k", linestyle="--", label="Idéale (E = 1)")
plt.xticks(np_liste)
plt.ylim(0, 1.2)
plt.xlabel("Nombre de processus P")
plt.ylabel("Efficacité E(P)")
plt.title("Efficacité parallèle en fonction de P")
plt.grid(True, alpha=0.3)
plt.legend()
enregistrer("efficacite_vs_np.png")

print("3 graphes enregistrés dans " + DOSSIER + "/")
