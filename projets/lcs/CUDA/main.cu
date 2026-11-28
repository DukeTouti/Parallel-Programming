#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcs_cuda.cuh"
#include "sequences.cuh"

/* Usage :
   ./bin/lcs_cuda fichierX fichierY [tuiles|diagonales] [T] [fichier_resultat]
   ./bin/lcs_cuda m n [tuiles|diagonales] [T] [fichier_resultat]   (sequences aleatoires)
   T : cote de la tuile (tuiles) ou threads par bloc (diagonales)
   fichier_resultat (utilise par le jeu de test) :
     ligne 1 : longueur t_transfert t_phase1 t_phase2 t_total
     ligne 2 : la LCS */

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
static void ecrire_resultat(const char *chemin, int longueur, const char *S, const temps_cuda_t *temps) {
	FILE *f = fopen(chemin, "w");

	if (NULL == f) {
		perror(chemin);
		return;
	}

	fprintf(f, "%d %.9f %.9f %.9f %.9f\n", longueur, temps->t_transfert, temps->t_phase1, temps->t_phase2, temps->t_total);
	fprintf(f, "%s\n", S);
	fclose(f);
}

int main(int argc, char **argv) {
	char *X = NULL;
	char *Y = NULL;
	char *S = NULL;
	int m = 0;
	int n = 0;
	int methode = METHODE_TUILES;
	int T;
	int longueur;
	temps_cuda_t temps;

	if (argc < 3) {
		fprintf(stderr, "Usage : %s fichierX|m fichierY|n [tuiles|diagonales] [T] [fichier_resultat]\n", argv[0]);
		return 1;
	}

	if (argc > 3 && 0 == strcmp(argv[3], "diagonales")) {
		methode = METHODE_DIAGONALES;
	}
	T = (METHODE_DIAGONALES == methode) ? THREADS_DIAGONALES : TUILE_DEFAUT;
	if (argc > 4) {
		T = atoi(argv[4]);
	}
	if (T < 1 || T > 1024 || (METHODE_TUILES == methode && T > TUILE_MAX)) {
		fprintf(stderr, "Erreur : T invalide (threads <= 1024, tuile <= %d)\n", TUILE_MAX);
		return 1;
	}

	if (!charger_sequences(argv[1], argv[2], &X, &m, &Y, &n)) {
		return 1;
	}

	afficher_infos_gpu();
	VERIFIER_CUDA(cudaFree(0)); // initialisation du contexte CUDA avant la mesure

	longueur = lcs_cuda(X, Y, m, n, methode, T, &S, &temps);
	if (longueur < 0) {
		free(X);
		free(Y);
		return 1;
	}

	printf("m = %d | n = %d | methode = %s | T = %d\n", m, n,
		(METHODE_DIAGONALES == methode) ? "diagonales" : "tuiles", T);
	printf("longueur LCS : %d\n", longueur);
	if (longueur <= AFFICHAGE_MAX) {
		printf("LCS : %s\n", S);
	} else {
		printf("LCS : %.*s... (%d caracteres)\n", AFFICHAGE_MAX, S, longueur);
	}
	printf("transfert = %.6f s | phase 1 = %.6f s | phase 2 = %.6f s | total = %.6f s\n",
		temps.t_transfert, temps.t_phase1, temps.t_phase2, temps.t_total);

	if (argc > 5) {
		ecrire_resultat(argv[5], longueur, S, &temps);
	}

	free(X);
	free(Y);
	free(S);
	return 0;
}
