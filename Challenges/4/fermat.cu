#include <stdio.h>
#include <math.h>
#include <cuda.h>


__device__ int isPerfectSquare(unsigned long long n) {
    unsigned long long r = (unsigned long long)(sqrt((double)n));
    return (r * r == n);
}


__global__ void fermatKernel(unsigned long long ......,
                             unsigned long long ......,
                             unsigned long long ......,
                             unsigned long long ......)
{
    unsigned long long tid = ........................;
    unsigned long long x = ........................;

    unsigned long long y2 = x * x - N;
    if (............ && ............) {
      
        if (atomicCAS(resX, 0ULL, x) == 0ULL) {
            *resY = (unsigned long long)sqrt((double)y2);
        }
    }
}

int main() {
    unsigned long long N = 1000036000099;  
    unsigned long long start = ..................;

    unsigned long long hX = 0, hY = 0;
    unsigned long long *dX, *dY;

    cudaMalloc(&dX, sizeof(unsigned long long));
    cudaMalloc(&dY, sizeof(unsigned long long));
    cudaMemset(dX, 0, sizeof(unsigned long long));

     int threadsPerBlock = ............;

      int maxTests = ............;
    int blocks = ............;

    fermatKernel<<<blocks, threadsPerBlock>>>(......, ......, ......, ......);
    
    cudaDeviceSynchronize();

    cudaMemcpy(&hX, dX, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
    cudaMemcpy(&hY, dY, sizeof(unsigned long long), cudaMemcpyDeviceToHost);

    if (............) {
        printf("Factors: %llu and %llu\n", hX - hY, hX + hY);
    } else {
        printf("No factors found in range\n");
    }

    cudaFree(dX);
    cudaFree(dY);
    return 0;
}

