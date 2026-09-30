#include <stdio.h>
#include <mpi.h>
#include "Hello.h"

/*Affiche le rang, le nombre total de processus et la machine hote du processus courant*/
void afficher_hello(int rang, int taille) {
	int namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];

	MPI_Get_processor_name(processor_name, &namelen);

	printf("Hello, world, I am %d of %d running on the machine %s\n", rang, taille, processor_name);
}
