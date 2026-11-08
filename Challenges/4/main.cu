#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime.h>

#define NB_TESTS 10
#define REPS     3

__device__ int isPerfectSquare(unsigned long long n) {
	unsigned long long r = (unsigned long long)(sqrt((double)n));
	return (r * r == n);
}

__global__ void fermatKernel(unsigned long long N,
                             unsigned long long start,
                             int maxTests,
                             int threadsTotal,
                             unsigned long long *resX,
                             unsigned long long *resY)
{
	unsigned long long tid = blockIdx.x * blockDim.x + threadIdx.x;

	for (int i = (int)tid; i < maxTests; i += threadsTotal) {
		unsigned long long x  = start + i;
		unsigned long long y2 = x * x - N;
		if (isPerfectSquare(y2) && *resX == 0) {
			if (atomicCAS(resX, 0ULL, x) == 0ULL)
				*resY = (unsigned long long)sqrt((double)y2);
			break;
		}
	}
}

static double fermat_gpu(unsigned long long N, int maxTests,
                         int blocks, int threadsPerBlock,
                         unsigned long long *gx, unsigned long long *gy)
{
	unsigned long long start = (unsigned long long)ceil(sqrt((double)N));
	unsigned long long *dX, *dY;

	cudaMalloc(&dX, sizeof(unsigned long long));
	cudaMalloc(&dY, sizeof(unsigned long long));
	cudaMemset(dX, 0, sizeof(unsigned long long));
	cudaMemset(dY, 0, sizeof(unsigned long long));

	cudaEvent_t t0, t1;
	cudaEventCreate(&t0);
	cudaEventCreate(&t1);

	cudaEventRecord(t0);
	fermatKernel<<<blocks, threadsPerBlock>>>(N, start, maxTests,
	                                          blocks * threadsPerBlock, dX, dY);
	cudaEventRecord(t1);
	cudaEventSynchronize(t1);

	float ms = 0;
	cudaEventElapsedTime(&ms, t0, t1);

	cudaMemcpy(gx, dX, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
	cudaMemcpy(gy, dY, sizeof(unsigned long long), cudaMemcpyDeviceToHost);

	cudaFree(dX);
	cudaFree(dY);
	cudaEventDestroy(t0);
	cudaEventDestroy(t1);

	return (double)ms / 1000.0;
}

static int isPerfectSquareCPU(unsigned long long n) {
	unsigned long long r = (unsigned long long)sqrt((double)n);
	return (r * r == n);
}

static double fermat_cpu(unsigned long long N, int maxTests,
                         unsigned long long *gx, unsigned long long *gy)
{
	unsigned long long start = (unsigned long long)ceil(sqrt((double)N));
	*gx = 0; *gy = 0;

	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);

	for (int i = 0; i < maxTests; i++) {
		unsigned long long x  = start + i;
		unsigned long long y2 = x * x - N;
		if (isPerfectSquareCPU(y2)) {
			*gx = x;
			*gy = (unsigned long long)sqrt((double)y2);
			break;
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);
	return (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
}

int main(int argc, char **argv) {
	const char *outfile    = (argc > 1) ? argv[1] : "resultats.csv";
	int         maxTests   = (argc > 2) ? atoi(argv[2]) : 50000000;
	int         tpb        = (argc > 3) ? atoi(argv[3]) : 256;    // threads per block
	int         blocks     = (argc > 4) ? atoi(argv[4]) : 512;

	unsigned long long Ps[NB_TESTS] = {
		1000003ULL, 2000003ULL,   5000009ULL,   10000019ULL,   20000003ULL,
		50000009ULL, 100000007ULL, 200000003ULL, 500000009ULL, 1000000007ULL
	};
	unsigned long long Qs[NB_TESTS] = {
		1000033ULL, 2000033ULL,   5000029ULL,   10000079ULL,   20000033ULL,
		50000029ULL, 100000037ULL, 200000033ULL, 500000029ULL, 1000000033ULL
	};

	FILE *f = fopen(outfile, "a");
	if (!f) { perror("fopen"); return 1; }

	for (int t = 0; t < NB_TESTS; t++) {
		unsigned long long N = Ps[t] * Qs[t];
		unsigned long long gx, gy;

		/* -- CPU -- */
		double best_cpu = 1e30;
		for (int r = 0; r < REPS; r++) {
			double dt = fermat_cpu(N, maxTests, &gx, &gy);
			if (dt < best_cpu) best_cpu = dt;
		}
		double cpu_x = gx, cpu_y = gy;

		/* -- GPU -- */
		double best_gpu = 1e30;
		for (int r = 0; r < REPS; r++) {
			double dt = fermat_gpu(N, maxTests, blocks, tpb, &gx, &gy);
			if (dt < best_gpu) best_gpu = dt;
		}

		double speedup = best_cpu / (best_gpu + 1e-9);
		int    ok      = (gx != 0 && (gx - gy) * (gx + gy) == N);

		fprintf(f, "%d,%llu,%d,%d,%.6f,%.6f,%.4f,%d\n",
		        t + 1, N, tpb, blocks, best_cpu, best_gpu, speedup, ok);
		printf("Test %2d  N=%-25llu  blocks=%-4d tpb=%-4d  "
		       "CPU=%.4fs  GPU=%.4fs  Speedup=%.2fx  %s\n",
		       t + 1, N, blocks, tpb, best_cpu, best_gpu, speedup, ok ? "OK" : "ECHEC");
		(void)cpu_x; (void)cpu_y;
	}

	fclose(f);
	return 0;
}
