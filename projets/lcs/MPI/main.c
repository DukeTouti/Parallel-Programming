#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "lcs_mpi.h"
#include "sequences.h"

/* Usage : mpirun -np P ./lcs_mpi m n [hauteur_bloc] [csv]
   - m, n         : longueurs des sequences X et Y, generees aleatoirement
   - hauteur_bloc : nombre de lignes calculees entre deux envois au voisin (defaut : 64)
   - csv          : n'affiche qu'une ligne (pour le jeu de test) :
                    longueur,t_distribution,t_phase1,t_phase2,t_total,LCS */

#define HAUTEUR_BLOC_DEFAUT 64
#define AFFICHAGE_MAX 80

int main(int argc, char *argv[]) {
	int rang, taille;
	char *X = NULL;
	char *Y = NULL;
	char *S = NULL;
	int m, n;
	int hauteur_bloc;
	int mode_csv;
	int ok = 1;
	int longueur;
	temps_mpi_t temps;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	MPI_Comm_size(MPI_COMM_WORLD, &taille);

	if (argc < 3) {
		if (MAITRE == rang) {
			fprintf(stderr, "Usage : mpirun -np P %s m n [hauteur_bloc] [csv]\n", argv[0]);
		}
		MPI_Finalize();
		return 1;
	}

	m = atoi(argv[1]);
	n = atoi(argv[2]);
	hauteur_bloc = (argc > 3) ? atoi(argv[3]) : HAUTEUR_BLOC_DEFAUT;
	if (hauteur_bloc < 1) {
		hauteur_bloc = 1;
	}
	mode_csv = (argc > 4 && 0 == strcmp(argv[4], "csv"));

	/* ---------- Le maitre genere les sequences ---------- */
	if (MAITRE == rang) {
		ok = generer_sequences(m, n, &X, &Y);
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
		if (mode_csv) {
			printf("%d,%.9f,%.9f,%.9f,%.9f,%s\n", longueur, temps.t_distribution, temps.t_phase1, temps.t_phase2, temps.t_total, S);
		} else {
			printf("m = %d | n = %d | %d processus | hauteur de bloc = %d\n", m, n, taille, hauteur_bloc);
			printf("longueur LCS : %d\n", longueur);
			if (longueur <= AFFICHAGE_MAX) {
				printf("LCS : %s\n", S);
			} else {
				printf("LCS : %.*s... (%d caracteres)\n", AFFICHAGE_MAX, S, longueur);
			}
			printf("distribution = %.6f s | phase 1 = %.6f s (dont calcul %.6f s) | phase 2 = %.6f s | total = %.6f s\n",
				temps.t_distribution, temps.t_phase1, temps.t_calcul_phase1, temps.t_phase2, temps.t_total);
		}
	}

	free(X);
	free(Y);
	free(S);

	MPI_Finalize();
	return (longueur >= 0) ? 0 : 1;
}
