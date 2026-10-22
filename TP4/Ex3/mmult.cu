
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// a = b * c
 __global__ void mmult(float *a, float *b, float *c, int N)
{
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
    long int N = 4096;
    int T = 32;
    
    if (argc == 2)
        T = atoi(argv[1]);

    cudaEvent_t start, stop;
    float time, flop, gflops;
    
    float *a = (float *)malloc(N*N*sizeof(float));
    float *b = (float *)malloc(N*N*sizeof(float));
    float *c = (float *)malloc(N*N*sizeof(float));

    minit(a, b, c, N);
    
    // allocate device memory
    float *devPtrA, *devPtrB, *devPtrC;
    cudaMalloc((void**)&devPtrA, N*N*sizeof(float)); 
    cudaMalloc((void**)&devPtrB, N*N*sizeof(float)); 
    cudaMalloc((void**)&devPtrC, N*N*sizeof(float)); 

    // copy input arrays to the device meory    
    cudaMemcpy(devPtrB, b, N*N*sizeof(float),  cudaMemcpyHostToDevice); 
    cudaMemcpy(devPtrC, c, N*N*sizeof(float),  cudaMemcpyHostToDevice); 
    cudaMemset(devPtrA, 0, N*N*sizeof(float));

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
    cudaMemcpy(a, devPtrA, N*N*sizeof(float),  cudaMemcpyDeviceToHost); 

    mprint(a, N, 5);
    
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
    flop = N*N*N*2.0f;
    gflops = flop / time / 1E9;
    printf("sec = %.2f   GFLOPS = %.3f\n", time, gflops);

    return EXIT_SUCCESS;
}


