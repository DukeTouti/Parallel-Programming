#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcs_cuda.cuh"
#include "sequences.cuh"

/* Usage : ./lcs_cuda m n [tuiles|diagonales] [T] [csv]
   - m, n : longueurs des sequences X et Y, generees aleatoirement
   - T    : cote de la tuile (tuiles, defaut 32) ou threads par bloc (diagonales, defaut 256)
   - csv  : n'affiche qu'une ligne (pour le jeu de test) :
            longueur,t_transfert,t_phase1,t_phase2,t_total,LCS */

#define AFFICHAGE_MAX 80

int main(int argc, char **argv) {
	char *X = NULL;
	char *Y = NULL;
	char *S = NULL;
	int m, n;
	int methode = METHODE_TUILES;
	int T;
	int mode_csv;
	int longueur;
	temps_cuda_t temps;

	if (argc < 3) {
		fprintf(stderr, "Usage : %s m n [tuiles|diagonales] [T] [csv]\n", argv[0]);
		return 1;
	}

	m = atoi(argv[1]);
	n = atoi(argv[2]);
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
	mode_csv = (argc > 5 && 0 == strcmp(argv[5], "csv"));

	if (!generer_sequences(m, n, &X, &Y)) {
		return 1;
	}

	if (!mode_csv) {
		afficher_infos_gpu();
	}
	VERIFIER_CUDA(cudaFree(0)); // initialisation du contexte CUDA avant la mesure

	longueur = lcs_cuda(X, Y, m, n, methode, T, &S, &temps);
	if (longueur < 0) {
		free(X);
		free(Y);
		return 1;
	}

	if (mode_csv) {
		printf("%d,%.9f,%.9f,%.9f,%.9f,%s\n", longueur, temps.t_transfert, temps.t_phase1, temps.t_phase2, temps.t_total, S);
	} else {
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
	}

	free(X);
	free(Y);
	free(S);
	return 0;
}
