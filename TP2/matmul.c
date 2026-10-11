#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "matmul.h"

int A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];
static int C_ref[SIZE][SIZE];
static volatile int puit; // empeche le compilateur de supprimer le calcul de reference

void fill_matrix(int m[SIZE][SIZE]) {
	static int n = 0;
	int i, j;

	for (i = 0; i < SIZE; i++) {
		for (j = 0; j < SIZE; j++) {
			m[i][j] = n++;
		}
	}
}

void print_matrix(int m[SIZE][SIZE]) {
	int i, j = 0;

	for (i = 0; i < SIZE; i++) {
		printf("\n\t| ");
		for (j = 0; j < SIZE; j++) {
			printf("%2d ", m[i][j]);
		}
		printf("|");
	}
}

/* ---------- Reference sequentielle ---------- */

double mat_mul_seq(int repetitions) {
	int i, j, k, r;
	double debut = MPI_Wtime();

	for (r = 0; r < repetitions; r++) {
		for (i = 0; i < SIZE; i++) {
			for (j = 0; j < SIZE; j++) {
				C_ref[i][j] = 0;
				for (k = 0; k < SIZE; k++) {
					C_ref[i][j] += A[i][k] * B[k][j];
				}
			}
		}
		puit = C_ref[0][0];
	}
	return (MPI_Wtime() - debut) / repetitions;
}

/* --------------------- Version parallele --------------------- */

int mat_mul_mpi(int myrank, int P, int verifier, int affichage,
	double *t_total, double *t_calcul_local) {
	int from, to, i, j, k;
	int correct = 1;
	double debut, debut_calcul;

	from = myrank * SIZE/P;
	to = (myrank+1) * SIZE/P;

	if (myrank == 0) { // le rang 0 remplit A et B
		fill_matrix(A);
		fill_matrix(B);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	debut = MPI_Wtime();

	MPI_Bcast(B, SIZE*SIZE, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(A, SIZE*SIZE/P, MPI_INT, A[from], SIZE*SIZE/P, MPI_INT, 0, MPI_COMM_WORLD); // seule la bande utile de A est envoyee

	debut_calcul = MPI_Wtime();
	for (i = from; i < to; i++) {
		for (j = 0; j < SIZE; j++) {
			C[i][j] = 0;
			for (k = 0; k < SIZE; k++) {
				C[i][j] += A[i][k] * B[k][j];
			}
		}
	}
	*t_calcul_local = MPI_Wtime() - debut_calcul;

	MPI_Gather(C[from], SIZE*SIZE/P, MPI_INT, C, SIZE*SIZE/P, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);
	*t_total = MPI_Wtime() - debut;

	if (myrank == 0 && verifier) { // comparaison avec le produit sequentiel
		mat_mul_seq(1);
		for (i = 0; i < SIZE; i++) {
			for (j = 0; j < SIZE; j++) {
				if (C[i][j] != C_ref[i][j]) {
					correct = 0;
				}
			}
		}
	}

	if (myrank == 0 && affichage) {
		printf("\n\n");
		print_matrix(A);
		printf("\n\n\t       * \n");
		print_matrix(B);
		printf("\n\n\t       = \n");
		print_matrix(C);
		printf("\n\n");
	}

	return correct;
}
