#ifndef MATVEC_H
#define MATVEC_H

#include "matmul.h"

extern int x[SIZE], y[SIZE];

void fill_vector(int v[SIZE]);
void print_vector(int v[SIZE]);

/* Exo 2 : y = A * x, meme schema que mat_mul_mpi */
int mat_vec_mpi(int myrank, int P, int verifier, int affichage,
	double *t_total, double *t_calcul_local);

/* Reference sequentielle sur A et x : temps moyen sur "repetitions" executions */
double mat_vec_seq(int repetitions);

#endif
