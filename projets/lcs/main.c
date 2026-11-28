#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "jeu_de_test.h"

int main(void) {
	int tailles[] = {10, 100, 500, 1000, 2000, 4000, 6000, 8000, 10000, 12000, 14000, 16000};
	int nb_tailles = sizeof(tailles) / sizeof(tailles[0]);
	int nb_tests = 0;
	int nb_echecs = 0;
	FILE *csv;
	int t;

	setvbuf(stdout, NULL, _IONBF, 0);
	srand(GRAINE);
	mkdir(DOSSIER_DONNEES, 0755); // sequences generees et resultats intermediaires

	csv = ouvrir_csv();
	if (NULL == csv) {
		return 1;
	}

	printf("=== Jeu de test LCS : sequences aleatoires sur \"%s\", %d repetitions par mesure ===\n", ALPHABET, NB_REPETITIONS);

	for (t = 0; t < nb_tailles; t++) {
		tester_taille(tailles[t], csv, &nb_tests, &nb_echecs);
	}

	fclose(csv);

	printf("\n=== Bilan : %d / %d tests reussis | resultats dans %s ===\n", nb_tests - nb_echecs, nb_tests, FICHIER_CSV);
	return (0 == nb_echecs) ? 0 : 1;
}
