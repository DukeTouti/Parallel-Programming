
#include <stdio.h>
#include <mpi.h>
int main(int argc, char *argv[])
{
	int rank, size, next, prev, message, tag = 201;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	next = (rank + 1) % size;
	prev = (rank + size - 1) % size;
	if (0 == rank) {
		message = 10;
		printf("Process 0 sending %d to %d, tag %d (%d processes in ring)\n",
		message, next, tag, size);
		MPI_Send(&message, 1, MPI_INT, next, tag, MPI_COMM_WORLD);
		printf("Process 0 sent to %d\n", next);
		}
	while (1) {
		MPI_Recv(&message, 1, MPI_INT, prev, tag, MPI_COMM_WORLD,
		MPI_STATUS_IGNORE);
	if (0 == rank) {
		--message;
		printf("Process 0 decremented value: %d\n", message);
	}
	MPI_Send(&message, 1, MPI_INT, next, tag, MPI_COMM_WORLD);
	if (0 == message) {
		printf("Process %d exiting\n", rank);

		break;
		}
	}
	if (0 == rank) {
	MPI_Recv(&message, 1, MPI_INT, prev, tag, MPI_COMM_WORLD,
	MPI_STATUS_IGNORE);
	}
	MPI_Finalize();
	return 0;
}
