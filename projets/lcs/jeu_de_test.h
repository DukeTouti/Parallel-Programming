#ifndef JEU_DE_TEST_H
#define JEU_DE_TEST_H

#include <stdio.h>

#define ALPHABET "ACGT"
#define GRAINE 42

/* Nombre d'executions par configuration, on garde la mediane */
#define NB_REPETITIONS 3

#define DOSSIER_DONNEES "donnees"
#define FICHIER_X "donnees/X.txt"
#define FICHIER_Y "donnees/Y.txt"
#define FICHIER_RESULTAT "donnees/resultat.txt"
#define FICHIER_CSV "resultats.csv"

#define EXEC_MPI "MPI/bin/lcs_mpi"
#define EXEC_CUDA "CUDA/bin/lcs_cuda"
#define MPIRUN "mpirun --oversubscribe"
#define HAUTEUR_BLOC 64

/* Resultat d'une execution MPI ou CUDA (lu dans FICHIER_RESULTAT) */
typedef struct {
	int longueur;
	double t_communication; // MPI : distribution de X et Y | CUDA : transfert vers le GPU
	double t_phase1;
	double t_phase2;
	double t_total;
	char *S;                // LCS trouvee
} resultat_t;

/*Ouvre le CSV (ecrase) et ecrit la ligne d'en-tete. Renvoie NULL en cas d'erreur.*/
FILE *ouvrir_csv(void);

/*Teste une taille n : genere X et Y aleatoires (n symboles), calcule la reference
  sequentielle, execute MPI pour chaque nombre de processus et CUDA pour chaque methode,
  verifie chaque LCS contre la reference, affiche une ligne par configuration et
  l'ajoute au CSV. Les compteurs de tests et d'echecs sont mis a jour.*/
void tester_taille(int n, FILE *csv, int *nb_tests, int *nb_echecs);

#endif /* JEU_DE_TEST_H */
