#include "benchmark.cuh"
#include "somme.cuh"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define REPETITIONS 5
#define TOLERANCE   1e-9   /* erreur relative max (ordre d'addition different) */

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

static int verifier(double ref, double res) {
	double err = fabs(ref - res) / fabs(ref);
	if (err > TOLERANCE) {
		printf("  ECHEC : ref=%.6f gpu=%.6f (err rel=%.2e)\n", ref, res, err);
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------------ */
/* Benchmark principal                                                  */
/* ------------------------------------------------------------------ */

int benchmark(int N, const char *csv_path) {
	int i, r;
	double *A;
	double s_cpu, s_gpu;
	double t_cpu, t_N1, t_NtT, t_1N;
	FILE *csv;

	A = (double *)malloc(N * sizeof(double));

	srand(1);
	for (i = 0; i < N; i++)
		A[i] = (double)rand() / RAND_MAX;

	/* initialisation du contexte CUDA (evite de fausser la 1ere mesure) */
	cudaFree(0);

	/* CPU reference */
	t_cpu = sum_cpu(N, A, &s_cpu, REPETITIONS);

	/* GPU <<<N, 1>>> */
	t_N1 = 0.0;
	for (r = 0; r < REPETITIONS; r++)
		t_N1 += sum_gpu_N1(N, A, &s_gpu);
	t_N1 /= REPETITIONS;
	printf("N=%-9d  CPU=%.6fs  GPU<<<N,1>>>=%.6fs   S=%.2f  GFLOPS=%.3f  %s\n",
	       N, t_cpu, t_N1, t_cpu / t_N1, (N - 1) / t_N1 / 1e9,
	       verifier(s_cpu, s_gpu) ? "OK" : "ECHEC");

	/* GPU <<<N/T, T>>> */
	t_NtT = 0.0;
	for (r = 0; r < REPETITIONS; r++)
		t_NtT += sum_gpu_NtT(N, A, &s_gpu);
	t_NtT /= REPETITIONS;
	printf("N=%-9d  CPU=%.6fs  GPU<<<N/T,T>>>=%.6fs S=%.2f  GFLOPS=%.3f  %s\n",
	       N, t_cpu, t_NtT, t_cpu / t_NtT, (N - 1) / t_NtT / 1e9,
	       verifier(s_cpu, s_gpu) ? "OK" : "ECHEC");

	/* GPU <<<1, N>>> — limite hardware 1024 threads/bloc */
	if (N <= MAX_THREADS_PER_BLOCK) {
		t_1N = 0.0;
		for (r = 0; r < REPETITIONS; r++)
			t_1N += sum_gpu_1N(N, A, &s_gpu);
		t_1N /= REPETITIONS;
		printf("N=%-9d  CPU=%.6fs  GPU<<<1,N>>>=%.6fs   S=%.2f  GFLOPS=%.3f  %s\n",
		       N, t_cpu, t_1N, t_cpu / t_1N, (N - 1) / t_1N / 1e9,
		       verifier(s_cpu, s_gpu) ? "OK" : "ECHEC");
	}

	csv = ouvrir_csv(csv_path);
	if (csv) {
		ecrire_ligne(csv, N, "N_1",   t_cpu, t_N1);
		ecrire_ligne(csv, N, "N_T_T", t_cpu, t_NtT);
		if (N <= MAX_THREADS_PER_BLOCK)
			ecrire_ligne(csv, N, "1_N", t_cpu, t_1N);
		fclose(csv);
	}

	free(A);
	return 0;
}
