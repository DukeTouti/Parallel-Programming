#ifndef JEU_DE_TEST_H
#define JEU_DE_TEST_H

#include <stdio.h>

/* Memes alphabet et graine que MPI/sequences.h et CUDA/sequences.cuh : le jeu de test
   regenere exactement les sequences calculees par les deux versions */
#define ALPHABET "ACGT"
#define GRAINE 42

/* Nombre d'executions par configuration, on garde la mediane */
#define NB_REPETITIONS 3

#define FICHIER_CSV "resultats.csv"
#define EXEC_MPI "MPI/lcs_mpi"
#define EXEC_CUDA "CUDA/lcs_cuda"
#define MPIRUN "mpirun --oversubscribe"
#define HAUTEUR_BLOC 64

/* Resultat d'une execution MPI ou CUDA (lu sur sa sortie en mode csv) */
typedef struct {
	int longueur;
	double t_communication; // MPI : distribution de X et Y | CUDA : transfert vers le GPU
	double t_phase1;
	double t_phase2;
	double t_total;
} resultat_t;

/*Ouvre le CSV (ecrase) et ecrit la ligne d'en-tete. Renvoie NULL en cas d'erreur.*/
FILE *ouvrir_csv(void);

/*Teste une taille n : regenere X et Y (n symboles), calcule la reference sequentielle,
  execute MPI pour chaque nombre de processus et CUDA pour chaque methode, verifie
  chaque LCS contre la reference, affiche une ligne par configuration et l'ajoute
  au CSV. Les compteurs de tests et d'echecs sont mis a jour.*/
void tester_taille(int n, FILE *csv, int *nb_tests, int *nb_echecs);

#endif /* JEU_DE_TEST_H */
