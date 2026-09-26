#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  int rang, nb_processus;

  MPI_Init( &argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rang);
  MPI_Comm_size(MPI_COMM_WORLD, &nb_processus);

  if ( (rang % 2) == 0)
    printf("Hello, je suis le processus pair   %d\n", rang);
  else
    printf("Hello, je suis le processus impair %d\n", rang);

  MPI_Finalize();
  return 0;
}
