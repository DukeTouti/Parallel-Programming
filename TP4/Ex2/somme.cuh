#ifndef SOMME_CUH
#define SOMME_CUH

#ifndef T_BLOCK
#define T_BLOCK 512
#endif

#define MAX_THREADS_PER_BLOCK 1024

/* Reference CPU : retourne le temps moyen (s), la somme dans *res */
double sum_cpu(int N, const double *A, double *res, int repetitions);

/* GPU : 3 configurations de grille (temps kernel en s, somme dans *res) */
double sum_gpu_N1 (int N, const double *A, double *res); /* <<<N,   1   >>> */
double sum_gpu_1N (int N, const double *A, double *res); /* <<<1,   N   >>> */
double sum_gpu_NtT(int N, const double *A, double *res); /* <<<N/T, T   >>> */

#endif
