#include <stdlib.h>
#include <string.h>
#include "messages.h"

/*Genere le message apres srand(GRAINE)*/
char *generer_message(int longueur) {
	char *texte = malloc((size_t)longueur + 1);
	int taille_alphabet = (int)strlen(ALPHABET);
	int i;

	if (NULL == texte) {
		return NULL;
	}

	srand(GRAINE);
	for (i = 0; i < longueur; i++) {
		texte[i] = ALPHABET[rand() % taille_alphabet];
	}
	texte[longueur] = '\0';

	return texte;
}

/*FNV-1a : h = (h xor octet) * FNV_PREMIER pour chaque octet*/
uint64_t empreinte(const void *donnees, size_t taille) {
	const unsigned char *octets = donnees;
	uint64_t h = 14695981039346656037ULL; // base FNV-1a 64 bits
	size_t i;

	for (i = 0; i < taille; i++) {
		h ^= octets[i];
		h *= 1099511628211ULL; // nombre premier FNV 64 bits
	}

	return h;
}
