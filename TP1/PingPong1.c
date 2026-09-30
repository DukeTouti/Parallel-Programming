#include <stdio.h>
#include <mpi.h>
#include "PingPong1.h"

/*Processus 0 : envoie le tableau de valeurs au processus 1 (ping simple)*/
void ping_emetteur(double valeurs[], int nb_valeurs, int etiquette) {
	MPI_Send(valeurs, nb_valeurs, MPI_DOUBLE, 1, etiquette, MPI_COMM_WORLD);
}

/*Processus 1 : recoit le tableau de valeurs et affiche la derniere*/
void ping_recepteur(double valeurs[], int nb_valeurs, int etiquette) {
	MPI_Status statut;

	MPI_Recv(valeurs, nb_valeurs, MPI_DOUBLE, 0, etiquette, MPI_COMM_WORLD, &statut);

	printf("Moi, processus 1, j'ai recu %d valeurs (derniere = %g) du processus 0.\n",
		nb_valeurs, valeurs[nb_valeurs - 1]);
}
