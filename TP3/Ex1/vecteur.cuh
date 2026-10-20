#ifndef VECTEUR_CUH
#define VECTEUR_CUH

#ifndef T_BLOCK
#define T_BLOCK 512
#endif

#define MAX_THREADS_PER_BLOCK 1024

/* Reference CPU */
double vecadd_cpu(int N, float *A, float *B, float *C, int repetitions);

/* GPU : 3 configurations de grille */
double vecadd_gpu_N1 (int N, float *A, float *B, float *C); /* <<<N,   1   >>> */
double vecadd_gpu_1N (int N, float *A, float *B, float *C); /* <<<1,   N   >>> */
double vecadd_gpu_NtT(int N, float *A, float *B, float *C); /* <<<N/T, T   >>> */

#endif
