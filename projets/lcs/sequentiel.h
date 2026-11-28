#ifndef SEQUENTIEL_H
#define SEQUENTIEL_H

#include <stddef.h>

/* Indice de la case (i, j) d'une matrice stockee ligne par ligne de "largeur" colonnes */
#define IDX(i, j, largeur) ((size_t)(i) * (size_t)(largeur) + (size_t)(j))

/* max(a, b) du pseudo-code du sujet */
#define MAXIMUM(a, b) (((a) >= (b)) ? (a) : (b))

/*Phase 1 du sujet : alloue et remplit L[m+1][n+1], ou L[i][j] = longueur de la LCS
  de X[0..i-1] et Y[0..j-1]. Renvoie NULL si l'allocation echoue.*/
int *lcs_phase1(const char *X, const char *Y, int m, int n);

/*Phase 2 du sujet : remonte L depuis (m, n) et ecrit une LCS dans S
  (taille L[m][n] + 1). Renvoie la longueur de la LCS.*/
int lcs_phase2(const int *L, const char *X, const char *Y, int m, int n, char *S);

/*Renvoie 1 si S[0..k-1] est une sous-sequence de X[0..m-1], 0 sinon*/
int est_sous_sequence(const char *S, int k, const char *X, int m);

#endif /* SEQUENTIEL_H */
