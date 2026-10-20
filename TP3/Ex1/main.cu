#include <stdio.h>
#include <stdlib.h>
#include "benchmark.cuh"
#include "vecteur.cuh"

int main(int argc, char **argv) {
	int N = 720896;
	const char *csv = "resultats.csv";

	if (argc > 1) N   = atoi(argv[1]);
	if (argc > 2) csv = argv[2];

	if (N % T_BLOCK != 0) {
		fprintf(stderr, "N=%d doit etre divisible par T_BLOCK=%d\n", N, T_BLOCK);
		return 1;
	}

	return benchmark(N, csv);
}
