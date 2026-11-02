#include "benchmark.cuh"
#include "matrice.cuh"
#include <stdio.h>
#include <stdlib.h>

#define REPETITIONS 3

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
	size_t i, n = (size_t)N * N;
	for (i = 0; i < n; i++) {
		if (ref[i] != res[i]) {
			printf("  ECHEC a l'indice %zu : ref=%.2f gpu=%.2f\n",
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
	int r, rep_cpu;
	size_t taille = (size_t)N * N * sizeof(float);
	float *a_cpu, *a_gpu, *b, *c;
	double t_cpu, t_N1, t_NtT, t_1N;
	double flop = 2.0 * N * N * N;   /* 2 FLOP par iteration (mul + add) */
	FILE *csv;

	a_cpu = (float *)malloc(taille);
	a_gpu = (float *)malloc(taille);
	b     = (float *)malloc(taille);
	c     = (float *)malloc(taille);

	minit(a_gpu, b, c, N);

	/* initialisation du contexte CUDA (evite de fausser la 1ere mesure) */
	cudaFree(0);

	/* CPU reference (une seule mesure pour les grandes matrices : trop long) */
	rep_cpu = (N <= 512) ? REPETITIONS : 1;
	t_cpu = mmult_cpu(N, a_cpu, b, c, rep_cpu);

	/* GPU grille (N,N), bloc 1 */
	t_N1 = 0.0;
	for (r = 0; r < REPETITIONS; r++)
		t_N1 += mmult_gpu_N1(N, a_gpu, b, c);
	t_N1 /= REPETITIONS;
	printf("N=%-6d  CPU=%.6fs  GPU<<<N,1>>>=%.6fs   S=%.2f  GFLOPS=%.3f  %s\n",
	       N, t_cpu, t_N1, t_cpu / t_N1, flop / t_N1 / 1e9,
	       verifier(N, a_cpu, a_gpu) ? "OK" : "ECHEC");

	/* GPU grille (N/T,N), bloc T */
	t_NtT = 0.0;
	for (r = 0; r < REPETITIONS; r++)
		t_NtT += mmult_gpu_NtT(N, a_gpu, b, c);
	t_NtT /= REPETITIONS;
	printf("N=%-6d  CPU=%.6fs  GPU<<<N/T,T>>>=%.6fs S=%.2f  GFLOPS=%.3f  %s\n",
	       N, t_cpu, t_NtT, t_cpu / t_NtT, flop / t_NtT / 1e9,
	       verifier(N, a_cpu, a_gpu) ? "OK" : "ECHEC");

	/* GPU <<<1, N>>> — limite hardware 1024 threads/bloc */
	if (N <= MAX_THREADS_PER_BLOCK) {
		t_1N = 0.0;
		for (r = 0; r < REPETITIONS; r++)
			t_1N += mmult_gpu_1N(N, a_gpu, b, c);
		t_1N /= REPETITIONS;
		printf("N=%-6d  CPU=%.6fs  GPU<<<1,N>>>=%.6fs   S=%.2f  GFLOPS=%.3f  %s\n",
		       N, t_cpu, t_1N, t_cpu / t_1N, flop / t_1N / 1e9,
		       verifier(N, a_cpu, a_gpu) ? "OK" : "ECHEC");
	}

	csv = ouvrir_csv(csv_path);
	if (csv) {
		ecrire_ligne(csv, N, "N_1",   t_cpu, t_N1);
		ecrire_ligne(csv, N, "N_T_T", t_cpu, t_NtT);
		if (N <= MAX_THREADS_PER_BLOCK)
			ecrire_ligne(csv, N, "1_N", t_cpu, t_1N);
		fclose(csv);
	}

	free(a_cpu); free(a_gpu); free(b); free(c);
	return 0;
}
