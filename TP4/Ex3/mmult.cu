
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// a = b * c
// un thread calcule a[i + j*N] : i = blockIdx.x*blockDim.x + threadIdx.x, j = blockIdx.y
__global__ void mmult(float *a, float *b, float *c, int N)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y;
    int k;

    if (i < N && j < N)
    {
        float s = a[i + j*N];
        for (k = 0; k < N; k++)
            s += b[i + k*N] * c[k + j*N];
        a[i + j*N] = s;
    }
}

// a[][] = 0, b[][] = 1; c[][] = 1
void minit(float *a, float *b, float *c, int N)
{
    int i, j;
    
    for (j = 0; j < N; j++)
	for (i = 0; i < N; i++)
	{
	    a[i+N*j] = 0.0f;
	    b[i+N*j] = 1.0f;
	    c[i+N*j] = 1.0f;
	}
}

void mprint(float *a, int N, int M)
{
    int i, j;
    
    for (j = 0; j < M; j++)
    {
        for (i = 0; i < M; i++)
            printf("%.2f ", a[i+N*j]);
        printf("...\n");
    }
    printf("...\n");
}


int main(int argc, char* argv[])
{
    int N = 4096;
    int T = 32;
    
    if (argc > 1)
        T = atoi(argv[1]);
    if (argc > 2)
        N = atoi(argv[2]);

    if (N % T != 0)
    {
        fprintf(stderr, "N=%d doit etre divisible par T=%d\n", N, T);
        return EXIT_FAILURE;
    }

    cudaEvent_t start, stop;
    float time, flop, gflops;
    
    float *a = (float *)malloc((size_t)N*N*sizeof(float));
    float *b = (float *)malloc((size_t)N*N*sizeof(float));
    float *c = (float *)malloc((size_t)N*N*sizeof(float));

    minit(a, b, c, N);
    
    // allocate device memory
    float *devPtrA, *devPtrB, *devPtrC;
    cudaMalloc((void**)&devPtrA, (size_t)N*N*sizeof(float)); 
    cudaMalloc((void**)&devPtrB, (size_t)N*N*sizeof(float)); 
    cudaMalloc((void**)&devPtrC, (size_t)N*N*sizeof(float)); 

    // copy input arrays to the device memory
    cudaMemcpy(devPtrB, b, (size_t)N*N*sizeof(float),  cudaMemcpyHostToDevice); 
    cudaMemcpy(devPtrC, c, (size_t)N*N*sizeof(float),  cudaMemcpyHostToDevice); 
    cudaMemset(devPtrA, 0, (size_t)N*N*sizeof(float));

    // define grid and thread block sizes
    dim3 threads(T);
    dim3 grid(N/T, N);

    printf("matrix %ix%i\n", N, N);
    printf("grid %ix%i\n", grid.x, grid.y);
    printf("block %ix%ix%i\n", threads.x, threads.y, threads.z);

    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start, 0);

    // launch GPU kernel
    mmult<<<grid, threads>>>(devPtrA, devPtrB, devPtrC, N);

    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);

    // copy results to host
    cudaMemcpy(a, devPtrA, (size_t)N*N*sizeof(float),  cudaMemcpyDeviceToHost); 

    mprint(a, N, 5);
    printf("a[0] = %.2f (attendu : %d)  %s\n", a[0], N,
           a[0] == (float)N ? "OK" : "ECHEC");
    
    // free device memory
    cudaFree(devPtrA); 
    cudaFree(devPtrB); 
    cudaFree(devPtrC); 
    
    free(a);
    free(b);
    free(c);

    cudaEventElapsedTime(&time, start, stop);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    time /= 1000.0f;  // convert from milliseconds to seconds
    flop = 2.0f * N * N * N;
    gflops = flop / time / 1E9;
    printf("sec = %.2f   GFLOPS = %.3f\n", time, gflops);

    return EXIT_SUCCESS;
}
