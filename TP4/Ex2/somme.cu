#include "somme.cuh"
#include <stdio.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static double now_us(void) {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

/* atomicAdd sur double : natif a partir de sm_60, sinon boucle CAS */
__device__ static double atomic_add_double(double *addr, double val) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 600
	return atomicAdd(addr, val);
#else
	unsigned long long *p = (unsigned long long *)addr;
	unsigned long long old = *p, assumed;
	do {
		assumed = old;
		old = atomicCAS(p, assumed,
		                __double_as_longlong(val + __longlong_as_double(assumed)));
	} while (assumed != old);
	return __longlong_as_double(old);
#endif
}

/* ------------------------------------------------------------------ */
/* Kernel (identique pour les 3 configs, seul le lancement change)     */
/* ------------------------------------------------------------------ */

/* Chaque bloc reduit ses blockDim.x elements en memoire partagee (arbre),
   puis le thread 0 ajoute la somme partielle au resultat global. */
__global__ void kernel_sum(const double *A, double *res, int N) {
	extern __shared__ double sdata[];
	int tid = threadIdx.x;
	int i   = blockIdx.x * blockDim.x + tid;
	int p, s;

	sdata[tid] = (i < N) ? A[i] : 0.0;
	__syncthreads();

	/* plus petite puissance de 2 >= blockDim.x (blockDim.x quelconque) */
	p = 1;
	while (p < (int)blockDim.x) p <<= 1;

	for (s = p >> 1; s > 0; s >>= 1) {
		if (tid < s && tid + s < (int)blockDim.x)
			sdata[tid] += sdata[tid + s];
		__syncthreads();
	}

	if (tid == 0)
		atomic_add_double(res, sdata[0]);
}

/* ------------------------------------------------------------------ */
/* Reference CPU                                                        */
/* ------------------------------------------------------------------ */

double sum_cpu(int N, const double *A, double *res, int repetitions) {
	int r, i;
	double t0, total = 0.0, s = 0.0;

	for (r = 0; r < repetitions; r++) {
		s = 0.0;
		t0 = now_us();
		for (i = 0; i < N; i++)
			s += A[i];
		total += now_us() - t0;
	}
	*res = s;
	return (total / repetitions) / 1e6;
}

/* ------------------------------------------------------------------ */
/* GPU : allocation / liberation / mesure                              */
/* ------------------------------------------------------------------ */

static void device_alloc(int N, const double *A, double **dA, double **dRes) {
	cudaMalloc((void **)dA,   N * sizeof(double));
	cudaMalloc((void **)dRes, sizeof(double));
	cudaMemcpy(*dA, A, N * sizeof(double), cudaMemcpyHostToDevice);
	cudaMemset(*dRes, 0, sizeof(double));
}

static double launch_and_time(const double *dA, double *dRes, int N,
                               int blocks, int threads) {
	cudaEvent_t start, stop;
	cudaError_t err;
	float ms;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventRecord(start);
	kernel_sum<<<blocks, threads, threads * sizeof(double)>>>(dA, dRes, N);
	cudaEventRecord(stop);
	cudaEventSynchronize(stop);
	err = cudaGetLastError();
	if (err != cudaSuccess)
		fprintf(stderr, "Erreur kernel <<<%d,%d>>> : %s\n",
		        blocks, threads, cudaGetErrorString(err));
	cudaEventElapsedTime(&ms, start, stop);
	cudaEventDestroy(start);
	cudaEventDestroy(stop);
	return (double)ms / 1e3;
}

static void device_free(double *dA, double *dRes) {
	cudaFree(dA);
	cudaFree(dRes);
}

/* ------------------------------------------------------------------ */
/* Fonctions publiques GPU                                              */
/* ------------------------------------------------------------------ */

/* <<<N, 1>>> : N blocs d'un thread chacun (N atomicAdd sur le meme mot) */
double sum_gpu_N1(int N, const double *A, double *res) {
	double *dA, *dRes;
	double t;
	device_alloc(N, A, &dA, &dRes);
	t = launch_and_time(dA, dRes, N, N, 1);
	cudaMemcpy(res, dRes, sizeof(double), cudaMemcpyDeviceToHost);
	device_free(dA, dRes);
	return t;
}

/* <<<1, N>>> : 1 bloc de N threads (N <= 1024) */
double sum_gpu_1N(int N, const double *A, double *res) {
	double *dA, *dRes;
	double t;
	device_alloc(N, A, &dA, &dRes);
	t = launch_and_time(dA, dRes, N, 1, N);
	cudaMemcpy(res, dRes, sizeof(double), cudaMemcpyDeviceToHost);
	device_free(dA, dRes);
	return t;
}

/* <<<N/T, T>>> : equilibre blocs / threads */
double sum_gpu_NtT(int N, const double *A, double *res) {
	double *dA, *dRes;
	double t;
	device_alloc(N, A, &dA, &dRes);
	t = launch_and_time(dA, dRes, N, N / T_BLOCK, T_BLOCK);
	cudaMemcpy(res, dRes, sizeof(double), cudaMemcpyDeviceToHost);
	device_free(dA, dRes);
	return t;
}
