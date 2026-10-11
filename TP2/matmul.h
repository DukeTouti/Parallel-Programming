#ifndef MATMUL_H
#define MATMUL_H

#ifndef SIZE
#define SIZE 8
#endif

extern int A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];

void fill_matrix(int m[SIZE][SIZE]);
void print_matrix(int m[SIZE][SIZE]);

/*
 * C = A * B en parallele (code du prof), SIZE doit etre divisible par P
 * verifier : le rang 0 compare C avec le produit sequentiel
 * affichage : le rang 0 affiche A * B = C
 * Retourne 1 si le resultat est correct
 */
int mat_mul_mpi(int myrank, int P, int verifier, int affichage,
	double *t_total, double *t_calcul_local);

/* Reference sequentielle sur A et B : temps moyen sur "repetitions" executions */
double mat_mul_seq(int repetitions);

#endif
