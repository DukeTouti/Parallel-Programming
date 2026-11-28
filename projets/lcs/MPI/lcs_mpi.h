#ifndef LCS_MPI_H
#define LCS_MPI_H

#include <stddef.h>
#include <mpi.h>

#define MAITRE 0
#define ETIQUETTE_FRONTIERE 10
#define ETIQUETTE_TRACEBACK 20

/* Indice de la case (i, j) d'une matrice stockee ligne par ligne de "largeur" colonnes */
#define IDX(i, j, largeur) ((size_t)(i) * (size_t)(largeur) + (size_t)(j))

/* max(a, b) du pseudo-code du sujet */
#define MAXIMUM(a, b) (((a) >= (b)) ? (a) : (b))

/* Temps d'une execution parallele (en secondes, maximum sur les processus) */
typedef struct {
	double t_distribution;  // Bcast de X + Scatterv des colonnes de Y
	double t_phase1;        // construction de la matrice des scores (pipeline)
	double t_calcul_phase1; // calcul pur de la phase 1 (sans attente ni communication)
	double t_phase2;        // traceback distribue + rassemblement de la LCS
	double t_total;         // de la distribution au rassemblement
} temps_mpi_t;

/*Calcule le debut et la taille du segment attribue au processus de rang "rang"
  quand on repartit n elements sur nprocs processus (repartition la plus
  equilibree possible : les n % nprocs premiers recoivent un element de plus)*/
void calculer_segment(int n, int nprocs, int rang, int *debut, int *taille_segment);

/*Phase 1 parallele : decomposition de L par blocs de colonnes et pipeline par blocs
  de "hauteur_bloc" lignes (front d'onde). Le processus p possede ses nb_colonnes
  colonnes de L, plus en colonne locale 0 une copie de la derniere colonne du
  processus p-1, recue bloc par bloc. Alloue et renvoie la matrice locale
  (m+1) x (nb_colonnes+1), ou NULL si l'allocation echoue.
  *t_calcul recoit le temps de calcul pur du processus.*/
int *lcs_phase1_mpi(const char *X, const char *Y_local, int m, int nb_colonnes,
	int hauteur_bloc, int rang, int nprocs, MPI_Comm comm, double *t_calcul);

/*Phase 2 parallele : traceback distribue. Le dernier processus part de (m, n), remonte
  dans sa bande puis transmet la ligne i atteinte au processus de gauche, et ainsi de
  suite. La LCS est rassemblee sur le maitre (*S, allouee) par MPI_Gatherv.
  Renvoie la longueur de la LCS (significative sur le maitre uniquement).*/
int lcs_phase2_mpi(const int *L_local, const char *X, const char *Y_local, int m,
	int nb_colonnes, int rang, int nprocs, MPI_Comm comm, char **S);

/*Algorithme complet. X (longueur m) et Y (longueur n) ne sont significatifs que sur
  le maitre : diffusion de X, repartition des colonnes de Y, phases 1 et 2, LCS
  rassemblee sur le maitre (*S, a liberer). Les temps sont ecrits dans *temps sur le
  maitre. Renvoie la longueur de la LCS sur le maitre, -1 si une allocation echoue.*/
int lcs_mpi(const char *X, const char *Y, int m, int n, int hauteur_bloc,
	MPI_Comm comm, char **S, temps_mpi_t *temps);

#endif /* LCS_MPI_H */
