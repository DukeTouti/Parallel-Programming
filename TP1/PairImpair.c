#include <stdio.h>
#include "PairImpair.h"

/*Retourne 1 si le rang est pair, 0 sinon*/
int est_pair(int rang) {
	return (rang % 2) == 0;
}

/*Affiche un message different selon la parite du rang du processus*/
void afficher_parite(int rang) {
	if (est_pair(rang)) {
		printf("Je suis le processus pair de rang %d\n", rang);
	} else {
		printf("Je suis le processus impair de rang %d\n", rang);
	}
}
