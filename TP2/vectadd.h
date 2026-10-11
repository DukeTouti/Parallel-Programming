#ifndef VECTADD_H
#define VECTADD_H

#define MASTER 0
#ifndef ARRAY_SIZE
#define ARRAY_SIZE 8
#endif

/*
 * Addition parallele c = a + b (code du prof)
 * verifier : le maitre controle le resultat
 * affichage : le maitre affiche les triplets a b c
 * t_total : Scatter + calcul + Gather ; t_calcul_local : calcul seul
 * Retourne 1 si le resultat est correct
 */
int vect_add_mpi(int rank, int total_proc, int verifier, int affichage,
	double *t_total, double *t_calcul_local);

/* Reference sequentielle : temps moyen sur "repetitions" executions */
double vect_add_seq(int repetitions);

#endif
