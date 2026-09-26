#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

int main(int  argc,char** argv)
{
     int          myrank, nprocs, n, islave, master;
     double t0,t1;
     MPI_Status   status;
     int          ierr, resultlen;
     char         hostname[MPI_MAX_PROCESSOR_NAME];

     MPI_Init(& argc, & argv);

     MPI_Comm_rank(MPI_COMM_WORLD, & myrank);
     MPI_Comm_size(MPI_COMM_WORLD, & nprocs);

     MPI_Get_processor_name(hostname, & resultlen);

     t0 = MPI_Wtime();
     MPI_Barrier (MPI_COMM_WORLD);
     t1 = MPI_Wtime();

  
    // partie maitre: lit un entier sur l'entrée standard
    if ( myrank == 0 )
    {
        printf("input n:");
        scanf("%d",& n);

        for(islave=1 ; islave < nprocs ; islave++)
        {
            ierr = MPI_Send(& n, 1, MPI_INT, islave, 10, MPI_COMM_WORLD);

            if(ierr != 0) 
            {
	   	printf("coin ! slave %d -> erreur %d\n", islave,ierr);
                MPI_Abort ( MPI_COMM_WORLD, 99 );
            }
       
           printf("Master %d done sending %d to the slave %d\n", myrank, n, islave); }
    }
    // partie esclave: recoit le message avec l'entier
    else
    {
        master = 0;
        ierr = MPI_Recv (& n, 1, MPI_INT, master, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE );

        if(ierr == 0)
        {
            printf("input from master: %d\n", n);
        }
        else
        {
            printf("coin ! slave %d -> erreur %d\n", islave,ierr);
            MPI_Abort ( MPI_COMM_WORLD, 99 );
        }
	   printf("The slave % d done receiving %d from the master %d\n", myrank, n, master);

    }

    MPI_Finalize();

}
