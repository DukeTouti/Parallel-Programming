#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "vectadd.h"

static volatile int puit; // empeche le compilateur de supprimer le calcul de reference

/* ------------------------ Version parallele ------------------------- */

int vect_add_mpi(int rank, int total_proc, int verifier, int affichage,
	double *t_total, double *t_calcul_local) {
	int * a = NULL;
	int * b = NULL;
	int * c = NULL;
	int n_per_proc;
	int n = ARRAY_SIZE;
	int i;
	int good = 1;
	double debut, debut_calcul;

	int * ap;
	int * bp;
	int * cp;

	if (rank == MASTER) { // le maitre initialise les tableaux complets
		a = (int *) malloc(sizeof(int)*n);
		b = (int *) malloc(sizeof(int)*n);
		c = (int *) malloc(sizeof(int)*n);

		for (i = 0; i < n; i++) {
			a[i] = i;
		}
		for (i = 0; i < n; i++) {
			b[i] = i;
		}
	}

	n_per_proc = n/total_proc; // le nombre de processus doit diviser n

	ap = (int *) malloc(sizeof(int)*n_per_proc);
	bp = (int *) malloc(sizeof(int)*n_per_proc);
	cp = (int *) malloc(sizeof(int)*n_per_proc);

	MPI_Barrier(MPI_COMM_WORLD);
	debut = MPI_Wtime();

	MPI_Scatter(a, n_per_proc, MPI_INT, ap, n_per_proc, MPI_INT, MASTER, MPI_COMM_WORLD);
	MPI_Scatter(b, n_per_proc, MPI_INT, bp, n_per_proc, MPI_INT, MASTER, MPI_COMM_WORLD);

	debut_calcul = MPI_Wtime();
	for (i = 0; i < n_per_proc; i++) {
		cp[i] = ap[i] + bp[i];
	}
	*t_calcul_local = MPI_Wtime() - debut_calcul;

	MPI_Gather(cp, n_per_proc, MPI_INT, c, n_per_proc, MPI_INT, MASTER, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);
	*t_total = MPI_Wtime() - debut;

	if (rank == MASTER && verifier) { // verification par le maitre
		for (i = 0; i < n; i++) {
			if (c[i] != a[i] + b[i]) {
				printf("problem at index %d\n", i);
				good = 0;
				break;
			}
		}
		if (good && affichage) {
			printf("Values correct!\n");
			for (i = 0; i < n; i++) {
				printf("%d %d %d \n", a[i], b[i], c[i]);
			}
		}
	}

	if (rank == MASTER) {
		free(a);
		free(b);
		free(c);
	}
	free(ap);
	free(bp);
	free(cp);

	return good;
}

/* ---------- Reference sequentielle ---------- */

double vect_add_seq(int repetitions) {
	int n = ARRAY_SIZE;
	int i, r;
	int * a = (int *) malloc(sizeof(int)*n);
	int * b = (int *) malloc(sizeof(int)*n);
	int * c = (int *) malloc(sizeof(int)*n);
	double debut, temps;

	for (i = 0; i < n; i++) {
		a[i] = i;
		b[i] = i;
	}

	debut = MPI_Wtime();
	for (r = 0; r < repetitions; r++) {
		a[0] = r; // evite que le calcul soit identique a chaque tour
		for (i = 0; i < n; i++) {
			c[i] = a[i] + b[i];
		}
		puit = c[n/2];
	}
	temps = (MPI_Wtime() - debut) / repetitions;

	free(a);
	free(b);
	free(c);
	return temps;
}
