#include <stdio.h> 
#include <stdlib.h>
#include <sys/time.h>


__global__  void vecAdd(float* A, float* B, float* C) 
{ 
    // threadIdx.x is a built-in variable provided by CUDA at runtime 
    int i = blockIdx.x * blockDim.x + threadIdx.x; 
    C[i] = A[i] + B[i]; 
}

int  main(int argc, char **argv)
{
    int i, N = 720896;  /* default vector size */
    float *A, *devPtrA;
    float *B, *devPtrB; 
    float *C, *devPtrC; 

    /* check for user-supplied vector size */
    if (argc > 1) 
        N = atoi(argv[1]);

    printf("Running GPU vecAdd for %i elements\n", N);

     /* allocate host memory */
    A = (float*)malloc(N * sizeof(float));
    B = (float*)malloc(N * sizeof(float));
    C = (float*)malloc(N * sizeof(float));

    /* generate random data */
    for (i = 0; i < N; i++)
    {
        A[i] = (float)random();
        B[i] = (float)RAND_MAX - A[i];
    }

    /* ----- GPU add-on ------- */
    cudaMalloc((void**)&devPtrA, N * sizeof(float)); 
    cudaMalloc((void**)&devPtrB, N * sizeof(float)); 
    cudaMalloc((void**)&devPtrC, N * sizeof(float)); 

    cudaMemcpy(devPtrA, A, N * sizeof(float), cudaMemcpyHostToDevice); 
    cudaMemcpy(devPtrB, B, N * sizeof(float), cudaMemcpyHostToDevice); 

    /* call compute kernel */
	/* vecAdd(N, A, B, C); */
    vecAdd<<<N/512, 512>>>(devPtrA, devPtrB, devPtrC);
    
    cudaMemcpy(C, devPtrC, N * sizeof(float), cudaMemcpyDeviceToHost); 

    cudaFree(devPtrA); 
    cudaFree(devPtrB); 
    cudaFree(devPtrC); 
    /* ------------ */
        
    /* print out first 10 results */
    for (i = 0; i < 10; i++)
        printf("C[%i]=%.2f\n", i, C[i]);
  
    /* free allocated host memory */
    free(A); 
    free(B);
    free(C);
	
	return EXIT_SUCCESS;
}

