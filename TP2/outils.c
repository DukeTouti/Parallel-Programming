#include <stdio.h>
#include <mpi.h>
#include "outils.h"

double chronometre_max(double temps_local) {
	double temps_max = 0.0;

	MPI_Reduce(&temps_local, &temps_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	return temps_max;
}

FILE *ouvrir_csv(const char *chemin) {
	FILE *f = fopen(chemin, "a");

	if (f == NULL) {
		perror("ouverture du fichier CSV");
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	if (ftell(f) == 0) { // fichier vide : on ecrit l'en-tete
		fprintf(f, "operation,taille,nb_processus,t_seq,t_par,t_calcul\n");
	}
	return f;
}
