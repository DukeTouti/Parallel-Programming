#include <stdio.h>
#include <mpi.h>
#include "Ring.h"

/*Calcule les rangs du processus suivant et precedent dans l'anneau*/
void calculer_voisins(int rang, int taille, int *suivant, int *precedent) {
	*suivant = (rang + 1) % taille;
	*precedent = (rang + taille - 1) % taille;
}

/*Le processus 0 initialise le message et l'envoie au processus suivant*/
void envoyer_message_initial(int suivant, int message, int etiquette) {
	printf("Process 0 sending %d to %d, tag %d\n", message, suivant, etiquette);
	MPI_Send(&message, 1, MPI_INT, suivant, etiquette, MPI_COMM_WORLD);
	printf("Process 0 sent to %d\n", suivant);
}

/*Fait circuler le message dans l'anneau jusqu'a ce qu'il atteigne 0*/
void faire_circuler_message(int rang, int suivant, int precedent, int etiquette) {
	int message;

	while (1) {
		MPI_Recv(&message, 1, MPI_INT, precedent, etiquette, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		if (0 == rang) {
			--message;
			printf("Process 0 decremented value: %d\n", message);
		}

		MPI_Send(&message, 1, MPI_INT, suivant, etiquette, MPI_COMM_WORLD);

		if (0 == message) {
			printf("Process %d exiting\n", rang);
			break;
		}
	}
}

/*Le processus 0 recoit le dernier passage du message pour cloturer le tour*/
void recevoir_message_final(int precedent, int etiquette) {
	int message;

	MPI_Recv(&message, 1, MPI_INT, precedent, etiquette, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
