#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "benchmark.h"
#include "outils.h"
#include "vectadd.h"
#include "matvec.h"
#include "matmul.h"

#define REPETITIONS 3

/* ---------- Aiguillage selon le mode ---------- */

static int mode_valide(const char *mode) {
	return strcmp(mode, "vecteur") == 0 || strcmp(mode, "matvec") == 0
		|| strcmp(mode, "matmat") == 0;
}

static int executer(const char *mode, int rang, int nb_proc, int verifier,
	int affichage, double *t_total, double *t_calcul_local) {
	if (strcmp(mode, "vecteur") == 0) {
		return vect_add_mpi(rang, nb_proc, verifier, affichage, t_total, t_calcul_local);
	}
	if (strcmp(mode, "matvec") == 0) {
		return mat_vec_mpi(rang, nb_proc, verifier, affichage, t_total, t_calcul_local);
	}
	return mat_mul_mpi(rang, nb_proc, verifier, affichage, t_total, t_calcul_local);
}

static double executer_seq(const char *mode, int repetitions) {
	if (strcmp(mode, "vecteur") == 0) {
		return vect_add_seq(repetitions);
	}
	if (strcmp(mode, "matvec") == 0) {
		return mat_vec_seq(repetitions);
	}
	return mat_mul_seq(repetitions);
}

static int taille_du_mode(const char *mode) {
	return (strcmp(mode, "vecteur") == 0) ? ARRAY_SIZE : SIZE;
}

/* ---------- Benchmark ---------- */

int benchmark(const char *mode, const char *fichier_csv, int rang, int nb_proc) {
	double somme_total = 0.0, somme_calcul = 0.0, t_seq = 0.0;
	int correct = 1;
	int r;

	if (!mode_valide(mode)) {
		if (rang == 0) {
			fprintf(stderr, "Mode inconnu : %s\n", mode);
		}
		return 1;
	}

	for (r = 0; r < REPETITIONS; r++) {
		double t_total = 0.0, t_calcul_local = 0.0;
		int ok = executer(mode, rang, nb_proc, (r == 0), 0, &t_total, &t_calcul_local); // verification a la premiere repetition seulement
		double t_calcul = chronometre_max(t_calcul_local);

		if (rang == 0) {
			somme_total += t_total;
			somme_calcul += t_calcul;
			if (r == 0) {
				correct = ok;
			}
		}
	}

	if (rang == 0) {
		t_seq = executer_seq(mode, REPETITIONS);
		double t_par = somme_total / REPETITIONS;
		double t_calcul = somme_calcul / REPETITIONS;
		double acceleration = t_seq / t_par;
		FILE *csv = ouvrir_csv(fichier_csv);

		printf("%-8s n=%-9d P=%-2d t_seq=%.6f t_par=%.6f (calcul=%.6f) S=%.2f E=%.2f %s\n",
			mode, taille_du_mode(mode), nb_proc, t_seq, t_par, t_calcul,
			acceleration, acceleration / nb_proc, correct ? "OK" : "ECHEC");

		if (csv != NULL) {
			fprintf(csv, "%s,%d,%d,%.9f,%.9f,%.9f\n", mode, taille_du_mode(mode),
				nb_proc, t_seq, t_par, t_calcul);
			fclose(csv);
		}
	}
	return 0;
}

/* ---------- Jeux de tests ---------- */

int tests(int rang, int nb_proc) {
	const char *modes[3] = {"vecteur", "matvec", "matmat"};
	int nb_echecs = 0;
	int m;

	for (m = 0; m < 3; m++) {
		double t_total = 0.0, t_calcul_local = 0.0;
		int ok = executer(modes[m], rang, nb_proc, 1, 1, &t_total, &t_calcul_local);

		if (rang == 0) {
			printf("[test] %-8s taille=%d P=%d : %s\n", modes[m],
				taille_du_mode(modes[m]), nb_proc, ok ? "OK" : "ECHEC");
			if (!ok) {
				nb_echecs++;
			}
		}
	}

	MPI_Bcast(&nb_echecs, 1, MPI_INT, 0, MPI_COMM_WORLD); // tous les rangs recoivent le resultat
	return nb_echecs;
}
