#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include "Hello.h"
#include "PairImpair.h"
#include "commun.h"
#include "PingPong1.h"
#include "PingPong2.h"
#include "Ring.h"
#include "MasterSlave.h"
#include "SommeParallele.h"

/* Nombre de mesures repetees pour chaque point (seq et parallele) de
   l'exercice 5, dont on garde la mediane pour lisser le bruit de mesure
   (hiccups OS/scheduler) plutot que de se fier a une seule execution. */
#define NB_REPETITIONS 5

/*Compare deux doubles pour qsort*/
static int comparer_doubles(const void *a, const void *b) {
	double da = *(const double *)a;
	double db = *(const double *)b;

	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}

/*Renvoie la mediane d'un tableau de n doubles (trie une copie, ne modifie
  pas l'original). n doit etre >= 1.*/
static double mediane(double valeurs[], int n) {
	double *copie = malloc(n * sizeof(double));
	double resultat;
	int i;

	for (i = 0; i < n; i++) {
		copie[i] = valeurs[i];
	}

	qsort(copie, n, sizeof(double), comparer_doubles);

	if (n % 2 == 1) {
		resultat = copie[n / 2];
	} else {
		resultat = (copie[n / 2 - 1] + copie[n / 2]) / 2.0;
	}

	free(copie);
	return resultat;
}

int main(int argc, char *argv[]) {
	int rang, taille;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	MPI_Comm_size(MPI_COMM_WORLD, &taille);

	setvbuf(stdout, NULL, _IONBF, 0);

	/* ============================================
	   Exemple : Hello World
	   ============================================ */
	if (0 == rang) {
		printf("\n=== Hello World ===\n");
	}
	afficher_hello(rang, taille);
	MPI_Barrier(MPI_COMM_WORLD);

	/* ============================================
	   Exercice 1 : Pair / Impair
	   ============================================ */
	if (0 == rang) {
		printf("\n=== Exercice 1 : Pair / Impair ===\n");
	}
	afficher_parite(rang);
	MPI_Barrier(MPI_COMM_WORLD);

	/* ============================================
	   Exercice 2 : Ping Pong
	   ============================================ */
	if (0 == rang) {
		printf("\n=== Exercice 2 : Ping Pong ===\n");
	}
	if (taille < 2) {
		if (0 == rang) {
			printf("Il faut au moins 2 processus pour l'exercice Ping Pong.\n");
		}
	} else {
		double valeurs[NB_VALEURS];

		if (0 == rang) {
			printf("--- Programme 1 : ping simple ---\n");
		}
		if (0 == rang) {
			generer_valeurs(valeurs, NB_VALEURS);
			ping_emetteur(valeurs, NB_VALEURS, ETIQUETTE_PINGPONG);
		} else if (1 == rang) {
			ping_recepteur(valeurs, NB_VALEURS, ETIQUETTE_PINGPONG);
		}
		MPI_Barrier(MPI_COMM_WORLD);

		if (0 == rang) {
			printf("--- Programme 2 : ping pong avec chrono ---\n");
		}
		if (0 == rang) {
			generer_valeurs(valeurs, NB_VALEURS);
			pingpong_emetteur(valeurs, NB_VALEURS, ETIQUETTE_PINGPONG);
		} else if (1 == rang) {
			pingpong_recepteur(valeurs, NB_VALEURS, ETIQUETTE_PINGPONG);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	/* ============================================
	   Exercice 3 : Anneau (Ring)
	   ============================================ */
	if (0 == rang) {
		printf("\n=== Exercice 3 : Anneau ===\n");
	}
	{
		int suivant, precedent;

		calculer_voisins(rang, taille, &suivant, &precedent);

		if (0 == rang) {
			envoyer_message_initial(suivant, MESSAGE_INITIAL, ETIQUETTE_RING);
		}

		faire_circuler_message(rang, suivant, precedent, ETIQUETTE_RING);

		if (0 == rang) {
			recevoir_message_final(precedent, ETIQUETTE_RING);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	/* ============================================
	   Exercice 4 : Maitre / Esclave
	   ============================================ */
	if (0 == rang) {
		printf("\n=== Exercice 4 : Maitre / Esclave ===\n");
	}
	if (0 == rang) {
		maitre(rang, taille);
	} else {
		esclave(rang, 0);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	/* ============================================
	   Exercice 5 : Somme parallele d'un grand tableau
	   Mesure des performances (sequentiel vs parallele) pour
	   plusieurs tailles de tableau ET plusieurs nombres de processus,
	   le tout en une seule execution mpirun -np NPMAX : pour chaque
	   "nb de processus a tester" <= NPMAX, on forme un sous-communicateur
	   avec seulement les NPMAX premiers rangs et on l'utilise a la place
	   de MPI_COMM_WORLD. Resultats ecrits dans resultats_somme.csv.
	   ============================================ */
	if (0 == rang) {
		printf("\n=== Exercice 5 : Somme Parallele ===\n");
	}
	{
		int tailles_tableau[] = {10000, 100000, 500000, 1000000, 5000000, 10000000};
		int nb_tailles = sizeof(tailles_tableau) / sizeof(tailles_tableau[0]);
		int nprocs_a_tester[] = {1, 2, 4, 8, 16};
		int nb_nprocs_a_tester = sizeof(nprocs_a_tester) / sizeof(nprocs_a_tester[0]);
		int idx_taille, idx_np;
		FILE *csv = NULL;

		if (0 == rang) {
			csv = fopen("resultats_somme.csv", "w");
			if (NULL == csv) {
				perror("Erreur ouverture resultats_somme.csv");
			} else {
				fprintf(csv, "taille_tableau,nb_processus,temps_sequentiel,temps_parallele,acceleration\n");
			}
		}

		for (idx_taille = 0; idx_taille < nb_tailles; idx_taille++) {
			int n = tailles_tableau[idx_taille];
			int *tab = NULL;
			double temps_sequentiel;
			double debut, fin;
			long long somme_seq = 0;
			int rep;

			/* Reference sequentielle pour cette taille : NB_REPETITIONS mesures,
			   on garde la mediane pour lisser le bruit de mesure ponctuel. */
			if (0 == rang) {
				double temps_seq_essais[NB_REPETITIONS];

				tab = malloc(n * sizeof(int));
				generer_tableau_entiers(tab, n);

				for (rep = 0; rep < NB_REPETITIONS; rep++) {
					debut = MPI_Wtime();
					somme_seq = somme_sequentielle(tab, n);
					fin = MPI_Wtime();
					temps_seq_essais[rep] = fin - debut;
				}
				temps_sequentiel = mediane(temps_seq_essais, NB_REPETITIONS);
			}

			for (idx_np = 0; idx_np < nb_nprocs_a_tester; idx_np++) {
				int npTest = nprocs_a_tester[idx_np];
				MPI_Comm sous_comm;
				int couleur;

				if (npTest > taille) {
					continue; /* pas assez de processus lances pour ce test */
				}

				/* Forme un sous-groupe avec seulement les npTest premiers rangs */
				couleur = (rang < npTest) ? 0 : MPI_UNDEFINED;
				MPI_Comm_split(MPI_COMM_WORLD, couleur, rang, &sous_comm);

				if (rang < npTest) {
					long long somme_par = 0;
					double temps_par_essais[NB_REPETITIONS];
					double temps_parallele;

					/* NB_REPETITIONS mesures paralleles, on garde la mediane
					   plutot qu'une seule execution (evite qu'un hiccup
					   ponctuel de l'OS/scheduler fausse le resultat). */
					for (rep = 0; rep < NB_REPETITIONS; rep++) {
						MPI_Barrier(sous_comm);
						debut = MPI_Wtime();
						somme_par = somme_parallele(tab, n, rang, npTest, sous_comm);
						MPI_Barrier(sous_comm);
						fin = MPI_Wtime();
						temps_par_essais[rep] = fin - debut;
					}

					if (0 == rang) {
						double acceleration;

						temps_parallele = mediane(temps_par_essais, NB_REPETITIONS);
						acceleration = temps_sequentiel / temps_parallele;

						printf("n = %8d | %2d proc | seq = %8.5f s | par = %8.5f s | acc = %.2fx | %s\n",
							n, npTest, temps_sequentiel, temps_parallele, acceleration,
							(somme_seq == somme_par) ? "somme OK" : "SOMME DIFFERENTE : ERREUR");

						if (csv != NULL) {
							fprintf(csv, "%d,%d,%f,%f,%f\n", n, npTest, temps_sequentiel, temps_parallele, acceleration);
						}
					}

					MPI_Comm_free(&sous_comm);
				}
			}

			if (0 == rang) {
				free(tab);
			}
		}

		if (0 == rang && csv != NULL) {
			fclose(csv);
		}
	}

	MPI_Finalize();
	return 0;
}
