#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "lcs_mpi.h"
#include "sequences.h"

/* Usage :
   mpirun -np P ./bin/lcs_mpi fichierX fichierY [hauteur_bloc] [fichier_resultat]
   mpirun -np P ./bin/lcs_mpi m n [hauteur_bloc] [fichier_resultat]   (sequences aleatoires)
   fichier_resultat (utilise par le jeu de test) :
     ligne 1 : longueur t_distribution t_phase1 t_phase2 t_total
     ligne 2 : la LCS */

#define HAUTEUR_BLOC_DEFAUT 64
#define GRAINE 42
#define AFFICHAGE_MAX 80

/*Charge X et Y depuis deux fichiers, ou les genere si les arguments sont des tailles*/
static int charger_sequences(const char *arg_X, const char *arg_Y, char **X, int *m, char **Y, int *n) {
	if (est_entier(arg_X) && est_entier(arg_Y)) {
		*m = atoi(arg_X);
		*n = atoi(arg_Y);
		srand(GRAINE);
		*X = generer_sequence(*m);
		*Y = generer_sequence(*n);
	} else {
		*X = lire_sequence(arg_X, m);
		*Y = lire_sequence(arg_Y, n);
	}

	return (NULL != *X && NULL != *Y);
}

/*Ecrit les temps et la LCS dans le fichier resultat lu par le jeu de test*/
static void ecrire_resultat(const char *chemin, int longueur, const char *S, const temps_mpi_t *temps) {
	FILE *f = fopen(chemin, "w");

	if (NULL == f) {
		perror(chemin);
		return;
	}

	fprintf(f, "%d %.9f %.9f %.9f %.9f\n", longueur, temps->t_distribution, temps->t_phase1, temps->t_phase2, temps->t_total);
	fprintf(f, "%s\n", S);
	fclose(f);
}

int main(int argc, char *argv[]) {
	int rang, taille;
	char *X = NULL;
	char *Y = NULL;
	char *S = NULL;
	int m = 0;
	int n = 0;
	int hauteur_bloc;
	int ok = 1;
	int longueur;
	temps_mpi_t temps;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	MPI_Comm_size(MPI_COMM_WORLD, &taille);

	if (argc < 3) {
		if (MAITRE == rang) {
			fprintf(stderr, "Usage : mpirun -np P %s fichierX|m fichierY|n [hauteur_bloc] [fichier_resultat]\n", argv[0]);
		}
		MPI_Finalize();
		return 1;
	}

	hauteur_bloc = (argc > 3) ? atoi(argv[3]) : HAUTEUR_BLOC_DEFAUT;
	if (hauteur_bloc < 1) {
		hauteur_bloc = 1;
	}

	/* ---------- Le maitre charge les sequences ---------- */
	if (MAITRE == rang) {
		ok = charger_sequences(argv[1], argv[2], &X, &m, &Y, &n);
	}
	MPI_Bcast(&ok, 1, MPI_INT, MAITRE, MPI_COMM_WORLD);
	if (!ok) {
		MPI_Finalize();
		return 1;
	}

	/* ---------- Calcul de la LCS ---------- */
	longueur = lcs_mpi(X, Y, m, n, hauteur_bloc, MPI_COMM_WORLD, &S, &temps);

	/* ---------- Affichage ---------- */
	if (MAITRE == rang && longueur >= 0) {
		printf("m = %d | n = %d | %d processus | hauteur de bloc = %d\n", m, n, taille, hauteur_bloc);
		printf("longueur LCS : %d\n", longueur);
		if (longueur <= AFFICHAGE_MAX) {
			printf("LCS : %s\n", S);
		} else {
			printf("LCS : %.*s... (%d caracteres)\n", AFFICHAGE_MAX, S, longueur);
		}
		printf("distribution = %.6f s | phase 1 = %.6f s (dont calcul %.6f s) | phase 2 = %.6f s | total = %.6f s\n",
			temps.t_distribution, temps.t_phase1, temps.t_calcul_phase1, temps.t_phase2, temps.t_total);

		if (argc > 4) {
			ecrire_resultat(argv[4], longueur, S, &temps);
		}
	}

	free(X);
	free(Y);
	free(S);

	MPI_Finalize();
	return (longueur >= 0) ? 0 : 1;
}
