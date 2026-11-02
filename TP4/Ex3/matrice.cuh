#ifndef MATRICE_CUH
#define MATRICE_CUH

#ifndef T_BLOCK
#define T_BLOCK 128
#endif

#define MAX_THREADS_PER_BLOCK 1024

/* a[][] = 0, b[][] = 1, c[][] = 1 */
void minit(float *a, float *b, float *c, int N);

/* Reference CPU : a = b * c, retourne le temps moyen (s) */
double mmult_cpu(int N, float *a, float *b, float *c, int repetitions);

/* GPU : 3 configurations de grille (temps kernel en s, resultat dans a) */
double mmult_gpu_N1 (int N, float *a, float *b, float *c); /* grille (N,N),     bloc 1   */
double mmult_gpu_1N (int N, float *a, float *b, float *c); /* grille (1,1),     bloc N   */
double mmult_gpu_NtT(int N, float *a, float *b, float *c); /* grille (N/T,N),   bloc T   */

#endif
