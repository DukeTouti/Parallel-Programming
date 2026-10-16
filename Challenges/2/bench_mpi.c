#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "rsa.h"

/* ---------- Constantes ---------- */

#define NB_TACHES 16
#define MAITRE 0
#define NB_MESURES 5 // la médiane de NB_MESURES mesures est retenue
#define REPETITIONS_DEFAUT 100000
#define FICHIER_CSV "resultats.csv"

/* Tableau 1 du sujet : paires (p, q) */
static const long long premiers[NB_TACHES][2] = {
	{101, 113}, {107, 127}, {131, 137}, {139, 149},
	{151, 163}, {167, 173}, {179, 191}, {193, 197},
	{199, 211}, {223, 227}, {229, 233}, {239, 241},
	{251, 257}, {263, 269}, {271, 277}, {281, 283}
};

/* Nombres de processus testés (ceux supérieurs à la taille du communicateur sont ignorés) */
static const int liste_np[] = {1, 2, 4, 8};

/* ---------- Calcul ---------- */

/* Exécute la tâche numéro t (paire t % NB_TACHES) et retourne N + e + d (somme de contrôle) */
long long executer_tache(long long t) {
	long long N = 0;
	long long e = 0;
	long long d = 0;

	generate_rsa_keys(premiers[t % NB_TACHES][0], premiers[t % NB_TACHES][1], &N, &e, &d);
	return N + e + d;
}

/* Version séquentielle : les total tâches dans un seul processus */
long long calculer_sequentiel(long long total) {
	long long somme = 0;

	for (long long t = 0; t < total; t++) {
		somme += executer_tache(t);
	}
	return somme;
}

/* Version parallèle : distribution cyclique, la tâche t est traitée par le processus (t % taille) */
long long calculer_parallele(long long total, int rang, int taille) {
	long long somme = 0;

	for (long long t = rang; t < total; t += taille) {
		somme += executer_tache(t);
	}
	return somme;
}

/* ---------- Statistiques ---------- */

int comparer(const void *a, const void *b) {
	double x = *(const double *)a;
	double y = *(const double *)b;

	return (x > y) - (x < y);
}

/* Médiane d'un tableau de n valeurs (n impair) */
double mediane(double *valeurs, int n) {
	qsort(valeurs, n, sizeof(double), comparer);
	return valeurs[n / 2];
}

/* ---------- Mesures ---------- */

/* Temps séquentiel T1 (médiane) ; range la somme de contrôle de référence dans *somme_ref */
double mesurer_sequentiel(long long total, long long *somme_ref) {
	double temps[NB_MESURES];

	for (int m = 0; m < NB_MESURES; m++) {
		double debut = MPI_Wtime();
		*somme_ref = calculer_sequentiel(total);
		temps[m] = MPI_Wtime() - debut;
	}
	return mediane(temps, NB_MESURES);
}

/* Temps parallèle TP (médiane) avec np processus ; valide seulement sur le maître.
   Le temps inclut la synchronisation de départ et la collecte (MPI_Reduce).
   *correct passe à 0 si la somme de contrôle diffère de la version séquentielle. */
double mesurer_parallele(MPI_Comm comm, long long total, long long somme_ref, int *correct) {
	int rang;
	int taille;
	double temps[NB_MESURES];

	MPI_Comm_rank(comm, &rang);
	MPI_Comm_size(comm, &taille);

	for (int m = 0; m < NB_MESURES; m++) {
		long long somme_locale;
		long long somme_globale = 0;
		double debut;

		MPI_Barrier(comm);
		debut = MPI_Wtime();
		somme_locale = calculer_parallele(total, rang, taille);
		MPI_Reduce(&somme_locale, &somme_globale, 1, MPI_LONG_LONG, MPI_SUM, MAITRE, comm);
		temps[m] = MPI_Wtime() - debut;

		if (rang == MAITRE && somme_globale != somme_ref) {
			*correct = 0;
		}
	}
	return mediane(temps, NB_MESURES);
}

/* ---------- Programme principal ---------- */

int main(int argc, char **argv) {
	int rang;
	int taille;
	int correct = 1;
	long long repetitions = REPETITIONS_DEFAUT;
	long long total;
	long long somme_ref = 0;
	double t1 = 0.0;
	FILE *csv = NULL;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	MPI_Comm_size(MPI_COMM_WORLD, &taille);

	if (argc > 1) {
		repetitions = atoll(argv[1]);
	}
	if (repetitions < 1) {
		repetitions = REPETITIONS_DEFAUT;
	}
	total = repetitions * NB_TACHES;

	// Temps séquentiel : mesuré par le maître seul
	if (rang == MAITRE) {
		t1 = mesurer_sequentiel(total, &somme_ref);
		csv = fopen(FICHIER_CSV, "w");
		if (csv == NULL) {
			printf("Impossible d'ouvrir %s en ecriture\n", FICHIER_CSV);
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		fprintf(csv, "np,temps_s,acceleration,efficacite\n");
		fprintf(csv, "seq,%.6f,1.000000,1.000000\n", t1);

		printf("Charge : %lld repetitions x %d taches = %lld generations de cles\n", repetitions, NB_TACHES, total);
		printf("Mediane de %d mesures par configuration\n\n", NB_MESURES);
		printf("%-6s %-12s %-14s %-12s\n", "P", "Temps (s)", "Acceleration", "Efficacite");
		printf("%-6s %-12.6f %-14s %-12s\n", "seq", t1, "-", "-");
	}
	MPI_Barrier(MPI_COMM_WORLD);

	for (int k = 0; k < (int)(sizeof(liste_np) / sizeof(liste_np[0])); k++) {
		int np = liste_np[k];
		MPI_Comm sous;
		int couleur;

		if (np > taille) {
			if (rang == MAITRE) {
				printf("%-6d ignore (seulement %d processus lances)\n", np, taille);
			}
			continue;
		}

		// Seuls les np premiers processus participent à cette configuration
		couleur = (rang < np) ? 0 : MPI_UNDEFINED;
		MPI_Comm_split(MPI_COMM_WORLD, couleur, rang, &sous);

		if (sous != MPI_COMM_NULL) {
			double tp = mesurer_parallele(sous, total, somme_ref, &correct);

			if (rang == MAITRE) {
				double acceleration = t1 / tp;
				double efficacite = t1 / (np * tp);

				printf("%-6d %-12.6f %-14.4f %-12.4f\n", np, tp, acceleration, efficacite);
				fprintf(csv, "%d,%.6f,%.6f,%.6f\n", np, tp, acceleration, efficacite);
			}
			MPI_Comm_free(&sous);
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}

	if (rang == MAITRE) {
		fclose(csv);
		if (correct) {
			printf("\nVerification OK : meme somme de controle que la version sequentielle.\n");
		} else {
			printf("\nVerification KO : somme de controle differente du sequentiel.\n");
		}
		printf("Resultats ecrits dans %s\n", FICHIER_CSV);
	}

	MPI_Finalize();
	return 0;
}
