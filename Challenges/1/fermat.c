#include <stdio.h>
#include <math.h>
#include <mpi.h>

int isPerfectSquare(unsigned long long n) {
    unsigned long long r = (unsigned long long)sqrt((double)n);
    return (r * r == n);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    unsigned long long N = 1000000040000000231ULL; 
    unsigned long long start = (unsigned long long)ceil(sqrt((double)N));
    int maxTests = 1000000;

    unsigned long long localX = 0, localY = 0;

    
    for (int i = rank ; i < maxTests ; i += size) {
        unsigned long long x = start + i ;
        unsigned long long y2 = x * x - N;
        if ( isPerfectSquare(y2) && localX == 0) {
            localX = x ;
            localY = (unsigned long long)sqrt((double)y2) ;
            break;  
        }
    }

    // Reduce results: first non-zero value
    unsigned long long globalX = 0, globalY = 0;
    MPI_Allreduce(&localX, &globalX, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&localY, &globalY, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0) {
        if (globalX != 0)
            printf("MPI Factors: %llu and %llu\n", globalX - globalY, globalX + globalY);
        else
            printf("No factors found in range\n");
    }

    MPI_Finalize();
    return 0;
}

