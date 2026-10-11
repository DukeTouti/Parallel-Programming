#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "benchmark.h"
#include "vectadd.h"
#include "matmul.h"

int main(int argc, char *argv[]) {
	int total_proc;
	int rank;
	int code = 0;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &total_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (argc < 2) {
		if (rank == 0) {
			printf("Usage : mpirun -np P %s <vecteur|matvec|matmat|test> [resultats.csv]\n", argv[0]);
		}
		MPI_Finalize();
		return 1;
	}

	if (SIZE % total_proc != 0 || ARRAY_SIZE % total_proc != 0) { // restriction du prof : tailles divisibles par P
		if (rank == 0) {
			printf("Matrix size not divisible by number of processors\n");
		}
		MPI_Finalize();
		exit(-1);
	}

	if (strcmp(argv[1], "test") == 0) {
		code = (tests(rank, total_proc) == 0) ? 0 : 1;
	} else {
		code = benchmark(argv[1], (argc > 2) ? argv[2] : "resultats.csv", rank, total_proc);
	}

	MPI_Finalize();
	return code;
}
