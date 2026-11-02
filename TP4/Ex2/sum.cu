#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

/* atomicAdd sur double : natif a partir de sm_60, sinon boucle CAS */
__device__ double atomicAddDouble(double *addr, double val)
{
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

/* reduction par bloc en memoire partagee, puis 1 atomicAdd par bloc */
__global__ void sumKernel(double *A, double *res, int N)
{
    extern __shared__ double sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;
    int s;

    sdata[tid] = (i < N) ? A[i] : 0.0;
    __syncthreads();

    /* blockDim.x : puissance de 2 */
    for (s = blockDim.x / 2; s > 0; s >>= 1)
    {
        if (tid < s)
            sdata[tid] += sdata[tid + s];
        __syncthreads();
    }

    if (tid == 0)
        atomicAddDouble(res, sdata[0]);
}

double sum(double* A, int N)
{
    int i;
    double s = 0.0;

    for (i = 0; i < N; i++)
        s += A[i];

    return s;
}

int main(int argc, char **argv)
{
    int i, N = 2097152;   /* vector size */
    int T = 512;          /* threads par bloc (puissance de 2) */
    double *A, *devPtrA, *devPtrRes;
    double s_cpu, s_gpu = 0.0;

    struct timeval t1, t2;
    cudaEvent_t start, stop;
    float ms;
    double sec_cpu, sec_gpu;

    if (argc > 1) N = atoi(argv[1]);   /* get size of the vector */
    if (argc > 2) T = atoi(argv[2]);

    A = (double*)malloc(N * sizeof(double));

    srand(1);
    for (i = 0; i < N; i++)            /* generate random data */
        A[i] = (double)rand() / RAND_MAX;

    printf("Running GPU sum for %d elements (T=%d)\n", N, T);

    /* ----- CPU ------- */
    gettimeofday(&t1, NULL);
    s_cpu = sum(A, N);
    gettimeofday(&t2, NULL);
    sec_cpu = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1e6;

    /* ----- GPU add-on ------- */
    cudaMalloc((void**)&devPtrA,   N * sizeof(double));
    cudaMalloc((void**)&devPtrRes, sizeof(double));

    cudaMemcpy(devPtrA, A, N * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemset(devPtrRes, 0, sizeof(double));

    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);

    /* call compute kernel */
    sumKernel<<<(N + T - 1) / T, T, T * sizeof(double)>>>(devPtrA, devPtrRes, N);

    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&ms, start, stop);
    sec_gpu = ms / 1000.0;

    cudaMemcpy(&s_gpu, devPtrRes, sizeof(double), cudaMemcpyDeviceToHost);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    cudaFree(devPtrA);
    cudaFree(devPtrRes);
    /* ------------ */

    printf("CPU : sum=%.6f  sec=%f  GFLOPS=%.3f\n",
           s_cpu, sec_cpu, (N - 1) / sec_cpu / 1e9);
    printf("GPU : sum=%.6f  sec=%f  GFLOPS=%.3f\n",
           s_gpu, sec_gpu, (N - 1) / sec_gpu / 1e9);
    printf("Speedup = %.2f   %s\n", sec_cpu / sec_gpu,
           fabs(s_cpu - s_gpu) / fabs(s_cpu) < 1e-9 ? "OK" : "ECHEC");

    free(A);

    return EXIT_SUCCESS;
}
