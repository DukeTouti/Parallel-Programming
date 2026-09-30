#include <stdlib.h>
#include "commun.h"

/*Remplit un tableau de nb_valeurs reels aleatoires dans [0, 1[*/
void generer_valeurs(double valeurs[], int nb_valeurs) {
	int iter;

	for (iter = 0; iter < nb_valeurs; iter++) {
		valeurs[iter] = rand() / (RAND_MAX + 1.);
	}
}
