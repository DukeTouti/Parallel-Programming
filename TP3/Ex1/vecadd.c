#include <stdio.h> 
#include <stdlib.h>
#include <sys/time.h>


void vecAdd(int N, float* A, float* B, float* C);


int  main(int argc, char **argv)
{
    int i, N = 720896;  /* default vector size */
    float *A;
    float *B; 
    float *C; 

    struct timeval t1, t2;
    long msec1, msec2;
    float rt;

    /* check for user-supplied vector size */
    if (argc > 1) 
        N = atoi(argv[1]);

    printf("Running CPU vecAdd for %i elements\n", N);

    /* allocate memory */ 
    A = (float*)malloc(N * sizeof(float));
    B = (float*)malloc(N * sizeof(float));
    C = (float*)malloc(N * sizeof(float));

    for (i = 0; i < N; i++)  /* generate random data */
    {
        A[i] = (float)random();
        B[i] = (float)RAND_MAX - A[i];
    }

	/* timing */
	gettimeofday(&t1, NULL);
    msec1 = t1.tv_sec * 1000000 + t1.tv_usec;

    /* call compute kernel */
    vecAdd(N, A, B, C);

	/* timing */
    gettimeofday(&t2, NULL);
    msec2 = t2.tv_sec * 1E6 + t2.tv_usec;

    rt = (float)(msec2-msec1) / 1E6;
    printf("time=%.4f seconds, MFLOPS=%.1f\n", rt, (float)N/rt/1E6);	
	
    /* print out first 10 results */
    for (i = 0; i < 10; i++)
        printf("C[%i]=%.2f\n", i, C[i]);
  
    /* free allocated memory */
    free(A);
    free(B);
    free(C);
	
	return EXIT_SUCCESS;
}

void vecAdd(int N, float* A, float* B, float* C)
{ 
    int i;
    
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i]; 
    }
}
