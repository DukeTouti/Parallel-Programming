#include <stdio.h>
#include <mpi.h>
#include "MasterSlave.h"

/*Le maitre (rang 0) lit un entier sur l'entree standard et l'envoie a chaque esclave*/
void maitre(int myrank, int nprocs) {
	int n, islave, ierr;

	printf("input n:");
	scanf("%d", &n);

	for (islave = 1; islave < nprocs; islave++) {
		ierr = MPI_Send(&n, 1, MPI_INT, islave, ETIQUETTE_MASTERSLAVE, MPI_COMM_WORLD);

		if (ierr != 0) {
			printf("coin ! slave %d -> erreur %d\n", islave, ierr);
			MPI_Abort(MPI_COMM_WORLD, 99);
		}

		printf("Master %d done sending %d to the slave %d\n", myrank, n, islave);
	}
}

/*Chaque esclave recoit l'entier envoye par le maitre*/
void esclave(int myrank, int master) {
	int n, ierr;

	ierr = MPI_Recv(&n, 1, MPI_INT, master, ETIQUETTE_MASTERSLAVE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	if (ierr == 0) {
		printf("input from master: %d\n", n);
	} else {
		printf("coin ! slave %d -> erreur %d\n", myrank, ierr);
		MPI_Abort(MPI_COMM_WORLD, 99);
	}

	printf("The slave %d done receiving %d from the master %d\n", myrank, n, master);
}
