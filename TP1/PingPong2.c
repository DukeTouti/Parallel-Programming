#include <stdio.h>
#include <mpi.h>
#include "PingPong2.h"

/*Processus 0 : envoie le tableau au processus 1, recupere sa reponse et affiche le temps de communication*/
void pingpong_emetteur(double valeurs[], int nb_valeurs, int etiquette) {
	MPI_Status statut;
	double temps_debut, temps_fin;

	temps_debut = MPI_Wtime();
	MPI_Send(valeurs, nb_valeurs, MPI_DOUBLE, 1, etiquette, MPI_COMM_WORLD);
	MPI_Recv(valeurs, nb_valeurs, MPI_DOUBLE, 1, etiquette, MPI_COMM_WORLD, &statut);
	temps_fin = MPI_Wtime();

	printf("Moi, processus 0, j'ai envoye et recu %d valeurs (derniere = %g) "
		"du processus 1 en %f secondes.\n",
		nb_valeurs, valeurs[nb_valeurs - 1], temps_fin - temps_debut);
}

/*Processus 1 : recoit le tableau du processus 0 puis le lui renvoie tel quel*/
void pingpong_recepteur(double valeurs[], int nb_valeurs, int etiquette) {
	MPI_Status statut;

	MPI_Recv(valeurs, nb_valeurs, MPI_DOUBLE, 0, etiquette, MPI_COMM_WORLD, &statut);
	MPI_Send(valeurs, nb_valeurs, MPI_DOUBLE, 0, etiquette, MPI_COMM_WORLD);
}
