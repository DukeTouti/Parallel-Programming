#include <stdlib.h>
#include <string.h>
#include "sequences.cuh"

/*Alloue une sequence de "longueur" symboles tires aleatoirement dans ALPHABET*/
static char *generer_sequence(int longueur) {
	char *sequence = (char *)malloc((size_t)longueur + 1);
	int taille_alphabet = (int)strlen(ALPHABET);
	int i;

	if (NULL == sequence) {
		return NULL;
	}

	for (i = 0; i < longueur; i++) {
		sequence[i] = ALPHABET[rand() % taille_alphabet];
	}
	sequence[longueur] = '\0';

	return sequence;
}

/*Genere X puis Y apres srand(GRAINE)*/
int generer_sequences(int m, int n, char **X, char **Y) {
	srand(GRAINE);
	*X = generer_sequence(m);
	*Y = generer_sequence(n);

	return (NULL != *X && NULL != *Y);
}
