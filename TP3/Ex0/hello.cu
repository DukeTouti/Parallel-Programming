#include "hello.cuh"
#include <stdio.h>

__global__ void kernel(void) { }

void hello_cuda(void) {
	kernel<<<1, 1>>>();
	printf("Hello World!\n");
}
