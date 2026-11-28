#include <stdlib.h>
#include "sequentiel.h"

/*Phase 1 du sujet : construction de L[m+1][n+1] de bas en haut (boucles du sujet)*/
int *lcs_phase1(const char *X, const char *Y, int m, int n) {
	int largeur = n + 1;
	int *L = malloc(IDX(m + 1, 0, largeur) * sizeof(int));
	int i, j;

	if (NULL == L) {
		return NULL;
	}

	for (i = 0; i <= m; i++) {
		for (j = 0; j <= n; j++) {
			if (0 == i || 0 == j) {
				L[IDX(i, j, largeur)] = 0;
			} else if (X[i - 1] == Y[j - 1]) {
				L[IDX(i, j, largeur)] = L[IDX(i - 1, j - 1, largeur)] + 1;
			} else {
				L[IDX(i, j, largeur)] = MAXIMUM(L[IDX(i - 1, j, largeur)], L[IDX(i, j - 1, largeur)]);
			}
		}
	}

	return L;
}

/*Phase 2 du sujet : remontee de (m, n) vers le bord. La longueur finale etant connue
  (L[m][n]), chaque caractere est ecrit directement a sa place, de la fin vers le debut.*/
int lcs_phase2(const int *L, const char *X, const char *Y, int m, int n, char *S) {
	int largeur = n + 1;
	int longueur = L[IDX(m, n, largeur)];
	int k = longueur;
	int i = m;
	int j = n;

	S[longueur] = '\0';

	while (i > 0 && j > 0) {
		if (X[i - 1] == Y[j - 1]) {
			k--;
			S[k] = X[i - 1];
			i--;
			j--;
		} else if (L[IDX(i - 1, j, largeur)] >= L[IDX(i, j - 1, largeur)]) {
			i--;
		} else {
			j--;
		}
	}

	return longueur;
}

/*Renvoie 1 si S[0..k-1] est une sous-sequence de X[0..m-1], 0 sinon*/
int est_sous_sequence(const char *S, int k, const char *X, int m) {
	int i;
	int p = 0;

	for (i = 0; i < m && p < k; i++) {
		if (X[i] == S[p]) {
			p++;
		}
	}

	return (p == k);
}
