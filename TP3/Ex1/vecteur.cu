#include "vecteur.cuh"
#include <sys/time.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static double now_us(void) {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1e6 + t.tv_usec;
}

/* ------------------------------------------------------------------ */
/* Kernel (identique pour les 3 configs, seul le lancement change)     */
/* ------------------------------------------------------------------ */

__global__ void kernel_vecadd(float *A, float *B, float *C) {
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	C[i] = A[i] + B[i];
}

/* ------------------------------------------------------------------ */
/* Reference CPU                                                        */
/* ------------------------------------------------------------------ */

double vecadd_cpu(int N, float *A, float *B, float *C, int repetitions) {
	int r, i;
	double t0, total = 0.0;

	for (r = 0; r < repetitions; r++) {
		t0 = now_us();
		for (i = 0; i < N; i++)
			C[i] = A[i] + B[i];
		total += now_us() - t0;
	}
	return (total / repetitions) / 1e6;
}

/* ------------------------------------------------------------------ */
/* GPU : allocation / liberation / mesure                              */
/* ------------------------------------------------------------------ */

static void device_alloc(int N, float *A, float *B,
                          float **dA, float **dB, float **dC) {
	cudaMalloc((void **)dA, N * sizeof(float));
	cudaMalloc((void **)dB, N * sizeof(float));
	cudaMalloc((void **)dC, N * sizeof(float));
	cudaMemcpy(*dA, A, N * sizeof(float), cudaMemcpyHostToDevice);
	cudaMemcpy(*dB, B, N * sizeof(float), cudaMemcpyHostToDevice);
}

static double launch_and_time(float *dA, float *dB, float *dC,
                               int blocks, int threads) {
	cudaEvent_t start, stop;
	float ms;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventRecord(start);
	kernel_vecadd<<<blocks, threads>>>(dA, dB, dC);
	cudaEventRecord(stop);
	cudaEventSynchronize(stop);
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

/* <<<N, 1>>> : N blocs d'un thread chacun */
double vecadd_gpu_N1(int N, float *A, float *B, float *C) {
	float *dA, *dB, *dC;
	double t;
	device_alloc(N, A, B, &dA, &dB, &dC);
	t = launch_and_time(dA, dB, dC, N, 1);
	cudaMemcpy(C, dC, N * sizeof(float), cudaMemcpyDeviceToHost);
	device_free(dA, dB, dC);
	return t;
}

/* <<<1, N>>> : 1 bloc de N threads (N <= 1024) */
double vecadd_gpu_1N(int N, float *A, float *B, float *C) {
	float *dA, *dB, *dC;
	double t;
	device_alloc(N, A, B, &dA, &dB, &dC);
	t = launch_and_time(dA, dB, dC, 1, N);
	cudaMemcpy(C, dC, N * sizeof(float), cudaMemcpyDeviceToHost);
	device_free(dA, dB, dC);
	return t;
}

/* <<<N/T, T>>> : equilibre blocs / threads */
double vecadd_gpu_NtT(int N, float *A, float *B, float *C) {
	float *dA, *dB, *dC;
	double t;
	device_alloc(N, A, B, &dA, &dB, &dC);
	t = launch_and_time(dA, dB, dC, N / T_BLOCK, T_BLOCK);
	cudaMemcpy(C, dC, N * sizeof(float), cudaMemcpyDeviceToHost);
	device_free(dA, dB, dC);
	return t;
}
