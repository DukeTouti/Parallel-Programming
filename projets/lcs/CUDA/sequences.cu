#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sequences.cuh"

/*Lit une sequence dans un fichier texte*/
char *lire_sequence(const char *chemin, int *longueur) {
	FILE *f = fopen(chemin, "r");
	char *sequence;
	long taille_fichier;
	int c;
	int n = 0;

	if (NULL == f) {
		perror(chemin);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	taille_fichier = ftell(f);
	fseek(f, 0, SEEK_SET);

	sequence = (char *)malloc((size_t)taille_fichier + 1);
	if (NULL == sequence) {
		fclose(f);
		return NULL;
	}

	while ((c = fgetc(f)) != EOF) {
		if (!isspace(c)) {
			sequence[n] = (char)c;
			n++;
		}
	}
	sequence[n] = '\0';

	fclose(f);
	*longueur = n;
	return sequence;
}

/*Alloue une sequence aleatoire de "longueur" symboles*/
char *generer_sequence(int longueur) {
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

/*Renvoie 1 si la chaine ne contient que des chiffres, 0 sinon*/
int est_entier(const char *chaine) {
	int i;

	if ('\0' == chaine[0]) {
		return 0;
	}
	for (i = 0; chaine[i] != '\0'; i++) {
		if (!isdigit((unsigned char)chaine[i])) {
			return 0;
		}
	}
	return 1;
}
