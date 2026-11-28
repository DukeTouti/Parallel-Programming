#ifndef LCS_CUDA_CUH
#define LCS_CUDA_CUH

#include <stdio.h>
#include <stdlib.h>

/* Verification systematique des appels a l'API CUDA (fichier, ligne et message en cas d'erreur) */
#define VERIFIER_CUDA(appel) \
	do { \
		cudaError_t erreur_cuda = (appel); \
		if (cudaSuccess != erreur_cuda) { \
			fprintf(stderr, "Erreur CUDA %s:%d : %s\n", __FILE__, __LINE__, cudaGetErrorString(erreur_cuda)); \
			exit(EXIT_FAILURE); \
		} \
	} while (0)

/* Indice de la case (i, j) d'une matrice stockee ligne par ligne de "largeur" colonnes */
#define IDX(i, j, largeur) ((size_t)(i) * (size_t)(largeur) + (size_t)(j))

/* max(a, b) du pseudo-code du sujet */
#define MAXIMUM(a, b) (((a) >= (b)) ? (a) : (b))

/* Methodes de calcul de la phase 1 */
#define METHODE_DIAGONALES 0 // un lancement de noyau par anti-diagonale de cellules
#define METHODE_TUILES 1     // front d'onde de tuiles T x T calculees en memoire partagee

#define THREADS_DIAGONALES 256
#define TUILE_DEFAUT 32
#define TUILE_MAX 64

/* Temps d'une execution GPU (en secondes) */
typedef struct {
	double t_transfert; // copie de X et Y vers le GPU + initialisation des bords
	double t_phase1;    // construction de la matrice des scores
	double t_phase2;    // traceback sur le GPU + rapatriement de la LCS
	double t_total;     // t_transfert + t_phase1 + t_phase2
} temps_cuda_t;

/*Affiche le nom du GPU, sa capacite de calcul et sa memoire globale*/
void afficher_infos_gpu(void);

/*Algorithme complet sur le GPU. "methode" : METHODE_DIAGONALES (T = threads par bloc)
  ou METHODE_TUILES (T = cote de la tuile, 1 <= T <= TUILE_MAX). La LCS est ecrite dans
  *S (allouee, a liberer). Renvoie sa longueur, ou -1 si la memoire du GPU est insuffisante.*/
int lcs_cuda(const char *X, const char *Y, int m, int n, int methode, int T, char **S, temps_cuda_t *temps);

#endif /* LCS_CUDA_CUH */
