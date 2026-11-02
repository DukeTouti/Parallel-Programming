#include "matrice.cuh"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static double now_us(void) {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

void minit(float *a, float *b, float *c, int N) {
	int i, j;
	for (j = 0; j < N; j++)
		for (i = 0; i < N; i++) {
			a[i + N * j] = 0.0f;
			b[i + N * j] = 1.0f;
			c[i + N * j] = 1.0f;
		}
}

/* ------------------------------------------------------------------ */
/* Kernel (identique pour les 3 configs, seul le lancement change)     */
/* ------------------------------------------------------------------ */

/* a = b * c. Un thread calcule a[i + j*N] ; boucles "grid-stride" pour que
   le meme kernel couvre toute la matrice quelle que soit la grille. */
__global__ void kernel_mmult(float *a, float *b, float *c, int N) {
	int i, j, k;
	for (j = blockIdx.y; j < N; j += gridDim.y)
		for (i = blockIdx.x * blockDim.x + threadIdx.x; i < N;
		     i += gridDim.x * blockDim.x) {
			float s = a[i + j * N];
			for (k = 0; k < N; k++)
				s += b[i + k * N] * c[k + j * N];
			a[i + j * N] = s;
		}
}

/* ------------------------------------------------------------------ */
/* Reference CPU                                                        */
/* ------------------------------------------------------------------ */

double mmult_cpu(int N, float *a, float *b, float *c, int repetitions) {
	int r, i, j, k;
	double t0, total = 0.0;

	for (r = 0; r < repetitions; r++) {
		memset(a, 0, (size_t)N * N * sizeof(float));
		t0 = now_us();
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				for (i = 0; i < N; i++)
					a[i + j * N] += b[i + k * N] * c[k + j * N];
		total += now_us() - t0;
	}
	return (total / repetitions) / 1e6;
}

/* ------------------------------------------------------------------ */
/* GPU : allocation / liberation / mesure                              */
/* ------------------------------------------------------------------ */

static void device_alloc(int N, float *b, float *c,
                          float **dA, float **dB, float **dC) {
	size_t taille = (size_t)N * N * sizeof(float);
	cudaMalloc((void **)dA, taille);
	cudaMalloc((void **)dB, taille);
	cudaMalloc((void **)dC, taille);
	cudaMemcpy(*dB, b, taille, cudaMemcpyHostToDevice);
	cudaMemcpy(*dC, c, taille, cudaMemcpyHostToDevice);
	cudaMemset(*dA, 0, taille);
}

static double launch_and_time(float *dA, float *dB, float *dC, int N,
                               dim3 grid, dim3 threads) {
	cudaEvent_t start, stop;
	cudaError_t err;
	float ms;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventRecord(start);
	kernel_mmult<<<grid, threads>>>(dA, dB, dC, N);
	cudaEventRecord(stop);
	cudaEventSynchronize(stop);
	err = cudaGetLastError();
	if (err != cudaSuccess)
		fprintf(stderr, "Erreur kernel (%d,%d)x(%d) : %s\n",
		        grid.x, grid.y, threads.x, cudaGetErrorString(err));
	cudaEventElapsedTime(&ms, start, stop);
	cudaEventDestroy(start);
	cudaEventDestroy(stop);
	return (double)ms / 1e3;
}

static void device_free(float *dA, float *dB, float *dC) {
	cudaFree(dA);
	cudaFree(dB);
	cudaFree(dC);
}

/* ------------------------------------------------------------------ */
/* Fonctions publiques GPU                                              */
/* ------------------------------------------------------------------ */

/* "<<<N, 1>>>" : N x N blocs d'un thread, un bloc par element de a */
double mmult_gpu_N1(int N, float *a, float *b, float *c) {
	float *dA, *dB, *dC;
	double t;
	device_alloc(N, b, c, &dA, &dB, &dC);
	t = launch_and_time(dA, dB, dC, N, dim3(N, N), dim3(1));
	cudaMemcpy(a, dA, (size_t)N * N * sizeof(float), cudaMemcpyDeviceToHost);
	device_free(dA, dB, dC);
	return t;
}

/* "<<<1, N>>>" : 1 seul bloc de N threads (N <= 1024), chaque thread
   calcule une ligne i de a pour tous les j */
double mmult_gpu_1N(int N, float *a, float *b, float *c) {
	float *dA, *dB, *dC;
	double t;
	device_alloc(N, b, c, &dA, &dB, &dC);
	t = launch_and_time(dA, dB, dC, N, dim3(1, 1), dim3(N));
	cudaMemcpy(a, dA, (size_t)N * N * sizeof(float), cudaMemcpyDeviceToHost);
	device_free(dA, dB, dC);
	return t;
}

/* "<<<N/T, T>>>" : grille (N/T, N), blocs de T threads (config du template) */
double mmult_gpu_NtT(int N, float *a, float *b, float *c) {
	float *dA, *dB, *dC;
	double t;
	device_alloc(N, b, c, &dA, &dB, &dC);
	t = launch_and_time(dA, dB, dC, N, dim3(N / T_BLOCK, N), dim3(T_BLOCK));
	cudaMemcpy(a, dA, (size_t)N * N * sizeof(float), cudaMemcpyDeviceToHost);
	device_free(dA, dB, dC);
	return t;
}
