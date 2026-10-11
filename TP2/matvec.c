#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "matvec.h"

int x[SIZE], y[SIZE];
static int y_ref[SIZE];
static volatile int puit; // empeche le compilateur de supprimer le calcul de reference

void fill_vector(int v[SIZE]) {
	static int n = 0;
	int i;

	for (i = 0; i < SIZE; i++) {
		v[i] = n++;
	}
}

void print_vector(int v[SIZE]) {
	int i;

	printf("\n\t| ");
	for (i = 0; i < SIZE; i++) {
		printf("%2d ", v[i]);
	}
	printf("|");
}

/* ---------- Reference sequentielle ---------- */

double mat_vec_seq(int repetitions) {
	int i, k, r;
	double debut = MPI_Wtime();

	for (r = 0; r < repetitions; r++) {
		for (i = 0; i < SIZE; i++) {
			y_ref[i] = 0;
			for (k = 0; k < SIZE; k++) {
				y_ref[i] += A[i][k] * x[k];
			}
		}
		puit = y_ref[0];
	}
	return (MPI_Wtime() - debut) / repetitions;
}

/* ------------------------------ Version parallele ------------------------------ */

int mat_vec_mpi(int myrank, int P, int verifier, int affichage,
	double *t_total, double *t_calcul_local) {
	int from, to, i, k;
	int correct = 1;
	double debut, debut_calcul;

	from = myrank * SIZE/P;
	to = (myrank+1) * SIZE/P;

	if (myrank == 0) { // le rang 0 remplit A et x
		fill_matrix(A);
		fill_vector(x);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	debut = MPI_Wtime();

	MPI_Bcast(x, SIZE, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(A, SIZE*SIZE/P, MPI_INT, A[from], SIZE*SIZE/P, MPI_INT, 0, MPI_COMM_WORLD); // seule la bande utile de A est envoyee

	debut_calcul = MPI_Wtime();
	for (i = from; i < to; i++) {
		y[i] = 0;
		for (k = 0; k < SIZE; k++) {
			y[i] += A[i][k] * x[k];
		}
	}
	*t_calcul_local = MPI_Wtime() - debut_calcul;

	MPI_Gather(&y[from], SIZE/P, MPI_INT, y, SIZE/P, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);
	*t_total = MPI_Wtime() - debut;

	if (myrank == 0 && verifier) { // comparaison avec le produit sequentiel
		mat_vec_seq(1);
		for (i = 0; i < SIZE; i++) {
			if (y[i] != y_ref[i]) {
				correct = 0;
			}
		}
	}

	if (myrank == 0 && affichage) {
		printf("\n\n");
		print_matrix(A);
		printf("\n\n\t       * \n");
		print_vector(x);
		printf("\n\n\t       = \n");
		print_vector(y);
		printf("\n\n");
	}

	return correct;
}
