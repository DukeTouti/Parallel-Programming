#include <stdlib.h>
#include <mpi.h>
#include "SommeParallele.h"

/*Remplit un tableau de n entiers aleatoires (0 a 99)*/
void generer_tableau_entiers(int tab[], int n) {
	int i;

	for (i = 0; i < n; i++) {
		tab[i] = rand() % 100;
	}
}

/*Somme sequentielle (aucune communication, calcul complet sur un seul processus)*/
long long somme_sequentielle(int tab[], int n) {
	long long somme = 0;
	int i;

	for (i = 0; i < n; i++) {
		somme += tab[i];
	}

	return somme;
}

/*Calcule le debut et la taille du segment attribue au processus de rang "rang"
  quand on repartit n elements sur nprocs processus (repartition la plus equilibree possible)*/
void calculer_segment(int n, int nprocs, int rang, int *debut, int *taille_segment) {
	int base = n / nprocs;
	int reste = n % nprocs;

	if (rang < reste) {
		*taille_segment = base + 1;
		*debut = rang * (base + 1);
	} else {
		*taille_segment = base;
		*debut = reste * (base + 1) + (rang - reste) * base;
	}
}

/*Somme parallele : le maitre (rang 0 de comm) repartit tab[0..n[ entre tous
  les processus de comm, chacun somme son segment, puis le maitre collecte
  et additionne les sommes partielles.*/
long long somme_parallele(int tab[], int n, int rang, int nprocs, MPI_Comm comm) {
	int debut, taille_segment;
	long long somme_locale = 0;
	long long somme_totale;
	int i;

	calculer_segment(n, nprocs, rang, &debut, &taille_segment);

	if (0 == rang) {
		int r;

		/* Repartition : envoie a chaque esclave son morceau du tableau */
		for (r = 1; r < nprocs; r++) {
			int debut_r, taille_r;

			calculer_segment(n, nprocs, r, &debut_r, &taille_r);

			if (taille_r > 0) {
				MPI_Send(&tab[debut_r], taille_r, MPI_INT, r, ETIQUETTE_SOMME, comm);
			}
		}

		/* Le maitre calcule aussi la somme de son propre segment */
		for (i = debut; i < debut + taille_segment; i++) {
			somme_locale += tab[i];
		}
	} else {
		int *segment_local = malloc(taille_segment * sizeof(int));

		if (taille_segment > 0) {
			MPI_Recv(segment_local, taille_segment, MPI_INT, 0, ETIQUETTE_SOMME, comm, MPI_STATUS_IGNORE);
		}

		for (i = 0; i < taille_segment; i++) {
			somme_locale += segment_local[i];
		}

		free(segment_local);
	}

	somme_totale = somme_locale;

	if (0 == rang) {
		int r;

		/* Collecte des sommes partielles des esclaves */
		for (r = 1; r < nprocs; r++) {
			long long somme_r;

			MPI_Recv(&somme_r, 1, MPI_LONG_LONG, r, ETIQUETTE_SOMME, comm, MPI_STATUS_IGNORE);
			somme_totale += somme_r;
		}
	} else {
		MPI_Send(&somme_locale, 1, MPI_LONG_LONG, 0, ETIQUETTE_SOMME, comm);
	}

	return (0 == rang) ? somme_totale : 0;
}
