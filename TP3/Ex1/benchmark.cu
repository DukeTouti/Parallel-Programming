#include "benchmark.cuh"
#include "vecteur.cuh"
#include <stdio.h>
#include <stdlib.h>

#define REPETITIONS 5

/* ------------------------------------------------------------------ */
/* CSV                                                                  */
/* ------------------------------------------------------------------ */

static FILE *ouvrir_csv(const char *chemin) {
	FILE *f = fopen(chemin, "a");
	if (!f) { perror("CSV"); return NULL; }
	fseek(f, 0, SEEK_END);
	if (ftell(f) == 0)
		fprintf(f, "taille,config,t_cpu,t_gpu\n");
	return f;
}

static void ecrire_ligne(FILE *f, int N, const char *config,
                          double t_cpu, double t_gpu) {
	fprintf(f, "%d,%s,%.9f,%.9f\n", N, config, t_cpu, t_gpu);
}

/* ------------------------------------------------------------------ */
/* Verification                                                         */
/* ------------------------------------------------------------------ */

static int verifier(int N, float *ref, float *res) {
	int i;
	for (i = 0; i < N; i++) {
		if (ref[i] != res[i]) {
			printf("  ECHEC a l'indice %d : ref=%.2f gpu=%.2f\n",
			       i, ref[i], res[i]);
			return 0;
		}
	}
	return 1;
}

/* ------------------------------------------------------------------ */
/* Benchmark principal                                                  */
/* ------------------------------------------------------------------ */

int benchmark(int N, const char *csv_path) {
	int i, r;
	float *A, *B, *C_cpu, *C_gpu;
	double t_cpu, t_N1, t_NtT, t_1N;
	FILE *csv;

	A     = (float *)malloc(N * sizeof(float));
	B     = (float *)malloc(N * sizeof(float));
	C_cpu = (float *)malloc(N * sizeof(float));
	C_gpu = (float *)malloc(N * sizeof(float));

	for (i = 0; i < N; i++) {
		A[i] = (float)random();
		B[i] = (float)RAND_MAX - A[i];
	}

	/* CPU reference */
	t_cpu = vecadd_cpu(N, A, B, C_cpu, REPETITIONS);

	/* GPU <<<N, 1>>> */
	t_N1 = 0.0;
	for (r = 0; r < REPETITIONS; r++)
		t_N1 += vecadd_gpu_N1(N, A, B, C_gpu);
	t_N1 /= REPETITIONS;
	printf("N=%-8d  CPU=%.6fs  GPU<<<N,1>>>=%.6fs   S=%.2f  MFLOPS=%.1f  %s\n",
	       N, t_cpu, t_N1, t_cpu / t_N1, (float)N / t_N1 / 1e6,
	       verifier(N, C_cpu, C_gpu) ? "OK" : "ECHEC");

	/* GPU <<<N/T, T>>> */
	t_NtT = 0.0;
	for (r = 0; r < REPETITIONS; r++)
		t_NtT += vecadd_gpu_NtT(N, A, B, C_gpu);
	t_NtT /= REPETITIONS;
	printf("N=%-8d  CPU=%.6fs  GPU<<<N/T,T>>>=%.6fs S=%.2f  MFLOPS=%.1f  %s\n",
	       N, t_cpu, t_NtT, t_cpu / t_NtT, (float)N / t_NtT / 1e6,
	       verifier(N, C_cpu, C_gpu) ? "OK" : "ECHEC");

	/* GPU <<<1, N>>> — limite hardware 1024 threads/bloc */
	if (N <= MAX_THREADS_PER_BLOCK) {
		t_1N = 0.0;
		for (r = 0; r < REPETITIONS; r++)
			t_1N += vecadd_gpu_1N(N, A, B, C_gpu);
		t_1N /= REPETITIONS;
		printf("N=%-8d  CPU=%.6fs  GPU<<<1,N>>>=%.6fs   S=%.2f  MFLOPS=%.1f  %s\n",
		       N, t_cpu, t_1N, t_cpu / t_1N, (float)N / t_1N / 1e6,
		       verifier(N, C_cpu, C_gpu) ? "OK" : "ECHEC");
	}

	csv = ouvrir_csv(csv_path);
	if (csv) {
		ecrire_ligne(csv, N, "N_1",   t_cpu, t_N1);
		ecrire_ligne(csv, N, "N_T_T", t_cpu, t_NtT);
		if (N <= MAX_THREADS_PER_BLOCK)
			ecrire_ligne(csv, N, "1_N", t_cpu, t_1N);
		fclose(csv);
	}

	free(A); free(B); free(C_cpu); free(C_gpu);
	return 0;
}
