#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define NB_TESTS 10
#define REPS 3   // on répète chaque mesure et on garde le meilleur temps

int isPerfectSquare(unsigned long long n) {
    unsigned long long r = (unsigned long long)sqrt((double)n);
    return (r * r == n);
}

// Même logique que fermat.c, mais paramétrée par N et maxTests.
// Retourne x et y dans *gx et *gy (identiques sur tous les processus).
void fermat_mpi(unsigned long long N, int maxTests, int fullScan, int rank, int size,
                unsigned long long *gx, unsigned long long *gy) {
    unsigned long long start = (unsigned long long)ceil(sqrt((double)N));
    unsigned long long localX = 0, localY = 0;

    for (int i = rank; i < maxTests; i += size) {
        unsigned long long x = start + i;
        unsigned long long y2 = x * x - N;
        if (isPerfectSquare(y2) && localX == 0) {
            localX = x;
            localY = (unsigned long long)sqrt((double)y2);
            if (!fullScan) break;   // mode 0 : comportement exact de fermat.c
        }
    }

    *gx = 0; *gy = 0;
    MPI_Allreduce(&localX, gx, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&localY, gy, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Usage : mpirun -np P ./bench resultats.csv [maxTests] [fullScan 0|1]
    const char *outfile = (argc > 1) ? argv[1] : "resultats.csv";
    int maxTests = (argc > 2) ? atoi(argv[2]) : 50000000;
    int fullScan = (argc > 3) ? atoi(argv[3]) : 1;

    // Les 10 cas de test de l'énoncé : N = p * q
    unsigned long long P[NB_TESTS] = {
        1000003ULL, 2000003ULL, 5000009ULL, 10000019ULL, 20000003ULL,
        50000009ULL, 100000007ULL, 200000003ULL, 500000009ULL, 1000000007ULL};
    unsigned long long Q[NB_TESTS] = {
        1000033ULL, 2000033ULL, 5000029ULL, 10000079ULL, 20000033ULL,
        50000029ULL, 100000037ULL, 200000033ULL, 500000029ULL, 1000000033ULL};

    FILE *f = NULL;
    if (rank == 0) {
        f = fopen(outfile, "a");
        if (!f) { perror("fopen"); MPI_Abort(MPI_COMM_WORLD, 1); }
    }

    for (int t = 0; t < NB_TESTS; t++) {
        unsigned long long N = P[t] * Q[t];
        double best = 1e30;
        unsigned long long gx = 0, gy = 0;

        for (int r = 0; r < REPS; r++) {
            MPI_Barrier(MPI_COMM_WORLD);
            double t0 = MPI_Wtime();
            fermat_mpi(N, maxTests, fullScan, rank, size, &gx, &gy);
            double local = MPI_Wtime() - t0;

            // Temps réel = celui du processus le plus lent
            double tmax;
            MPI_Reduce(&local, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            if (rank == 0 && tmax < best) best = tmax;
        }

        if (rank == 0) {
            int ok = (gx != 0 && (gx - gy) * (gx + gy) == N);
            fprintf(f, "%d,%llu,%d,%d,%.6f,%d\n", t + 1, N, size, maxTests, best, ok);
            printf("Test %2d  N=%llu  np=%d  t=%.4fs  %s  (%llu x %llu)\n",
                   t + 1, N, size, best, ok ? "OK" : "ECHEC", gx - gy, gx + gy);
        }
    }

    if (rank == 0) fclose(f);
    MPI_Finalize();
    return 0;
}
