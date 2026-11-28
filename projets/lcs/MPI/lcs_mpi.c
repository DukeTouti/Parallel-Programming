#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "lcs_mpi.h"

/*Repartition la plus equilibree possible de n elements sur nprocs processus*/
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

/*Calcule les lignes [ligne_debut, ligne_fin[ de la bande locale (colonnes 1..nb_colonnes) :
  meme recurrence que la phase 1 du sujet, la colonne 0 etant la frontiere recue*/
static void calculer_bloc(int *L, const char *X, const char *Y_local, int nb_colonnes,
	int ligne_debut, int ligne_fin) {
	int largeur = nb_colonnes + 1;
	int i, j;

	for (i = ligne_debut; i < ligne_fin; i++) {
		for (j = 1; j <= nb_colonnes; j++) {
			if (X[i - 1] == Y_local[j - 1]) {
				L[IDX(i, j, largeur)] = L[IDX(i - 1, j - 1, largeur)] + 1;
			} else {
				L[IDX(i, j, largeur)] = MAXIMUM(L[IDX(i - 1, j, largeur)], L[IDX(i, j - 1, largeur)]);
			}
		}
	}
}

/*Phase 1 parallele. Une portion de colonne de L (hauteur_bloc entiers espaces de "largeur")
  est decrite par un type derive MPI_Type_vector : elle est envoyee et recue directement
  dans la matrice, sans tampon. Les envois sont non bloquants (MPI_Isend) : les cases
  envoyees ne sont plus jamais modifiees, le processus enchaine donc sur le bloc suivant
  pendant que le message part (recouvrement calcul / communication).*/
int *lcs_phase1_mpi(const char *X, const char *Y_local, int m, int nb_colonnes,
	int hauteur_bloc, int rang, int nprocs, MPI_Comm comm, double *t_calcul) {
	int largeur = nb_colonnes + 1;
	int *L = calloc(IDX(m + 1, 0, largeur), sizeof(int)); // ligne 0 et colonne 0 du rang 0 a zero
	int nb_blocs = (m + hauteur_bloc - 1) / hauteur_bloc;
	int reste = m % hauteur_bloc;
	MPI_Datatype type_colonne_plein, type_colonne_reste;
	MPI_Request *requetes = NULL;
	int nb_requetes = 0;
	int bloc;
	double debut;

	*t_calcul = 0.0;

	if (NULL == L) {
		return NULL;
	}

	MPI_Type_vector(hauteur_bloc, 1, largeur, MPI_INT, &type_colonne_plein);
	MPI_Type_commit(&type_colonne_plein);
	if (reste > 0) { // dernier bloc incomplet si m n'est pas multiple de hauteur_bloc
		MPI_Type_vector(reste, 1, largeur, MPI_INT, &type_colonne_reste);
		MPI_Type_commit(&type_colonne_reste);
	}

	if (rang < nprocs - 1 && nb_blocs > 0) {
		requetes = malloc(nb_blocs * sizeof(MPI_Request));
	}

	for (bloc = 0; bloc < nb_blocs; bloc++) {
		int ligne_debut = 1 + bloc * hauteur_bloc;
		int ligne_fin = ligne_debut + hauteur_bloc;
		MPI_Datatype type_colonne = type_colonne_plein;

		if (ligne_fin > m + 1) {
			ligne_fin = m + 1;
			type_colonne = type_colonne_reste;
		}

		if (rang > 0) { // colonne frontiere du voisin de gauche pour ce bloc de lignes
			MPI_Recv(&L[IDX(ligne_debut, 0, largeur)], 1, type_colonne, rang - 1,
				ETIQUETTE_FRONTIERE, comm, MPI_STATUS_IGNORE);
		}

		debut = MPI_Wtime();
		calculer_bloc(L, X, Y_local, nb_colonnes, ligne_debut, ligne_fin);
		*t_calcul += MPI_Wtime() - debut;

		if (rang < nprocs - 1) { // notre derniere colonne devient la frontiere du voisin de droite
			MPI_Isend(&L[IDX(ligne_debut, nb_colonnes, largeur)], 1, type_colonne,
				rang + 1, ETIQUETTE_FRONTIERE, comm, &requetes[nb_requetes]);
			nb_requetes++;
		}
	}

	if (nb_requetes > 0) {
		MPI_Waitall(nb_requetes, requetes, MPI_STATUSES_IGNORE);
	}

	free(requetes);
	MPI_Type_free(&type_colonne_plein);
	if (reste > 0) {
		MPI_Type_free(&type_colonne_reste);
	}

	return L;
}

/*Phase 2 parallele. Chaque correspondance trouvee fait baisser L d'exactement 1 et les
  autres deplacements conservent L : les caracteres trouves dans une bande occupent donc
  les positions [L en sortie, L en entree[ de la LCS, connues sans communication.*/
int lcs_phase2_mpi(const int *L_local, const char *X, const char *Y_local, int m,
	int nb_colonnes, int rang, int nprocs, MPI_Comm comm, char **S) {
	int largeur = nb_colonnes + 1;
	int i, j;
	int valeur_entree, position;
	int nb_caracteres;
	char *fragment;
	int *comptes = NULL;
	int *deplacements = NULL;
	int longueur = 0;
	int p;

	if (nprocs - 1 == rang) {
		i = m;
	} else {
		MPI_Recv(&i, 1, MPI_INT, rang + 1, ETIQUETTE_TRACEBACK, comm, MPI_STATUS_IGNORE);
	}
	j = nb_colonnes;

	valeur_entree = L_local[IDX(i, j, largeur)];
	position = valeur_entree;
	fragment = malloc((size_t)valeur_entree + 1);

	while (i > 0 && j > 0) {
		if (X[i - 1] == Y_local[j - 1]) {
			position--;
			fragment[position] = X[i - 1];
			i--;
			j--;
		} else if (L_local[IDX(i - 1, j, largeur)] >= L_local[IDX(i, j - 1, largeur)]) {
			i--;
		} else {
			j--;
		}
	}

	if (rang > 0) { // colonne frontiere (j == 0) ou ligne 0 atteinte : on passe la main
		MPI_Send(&i, 1, MPI_INT, rang - 1, ETIQUETTE_TRACEBACK, comm);
	}

	nb_caracteres = valeur_entree - position; // fragment = positions [position, valeur_entree[ de la LCS

	if (MAITRE == rang) {
		comptes = malloc(nprocs * sizeof(int));
		deplacements = malloc(nprocs * sizeof(int));
	}

	MPI_Gather(&nb_caracteres, 1, MPI_INT, comptes, 1, MPI_INT, MAITRE, comm);
	MPI_Gather(&position, 1, MPI_INT, deplacements, 1, MPI_INT, MAITRE, comm);

	if (MAITRE == rang) {
		for (p = 0; p < nprocs; p++) {
			longueur += comptes[p];
		}
		*S = malloc((size_t)longueur + 1);
		(*S)[longueur] = '\0';
	}

	MPI_Gatherv(&fragment[position], nb_caracteres, MPI_CHAR,
		(MAITRE == rang) ? *S : NULL, comptes, deplacements, MPI_CHAR, MAITRE, comm);

	free(fragment);
	free(comptes);
	free(deplacements);

	return longueur;
}

/*Algorithme complet : distribution, phase 1, phase 2, rassemblement*/
int lcs_mpi(const char *X, const char *Y, int m, int n, int hauteur_bloc,
	MPI_Comm comm, char **S, temps_mpi_t *temps) {
	int rang, nprocs;
	int dimensions[2];
	char *X_local = NULL;
	char *Y_local = NULL;
	int *comptes = NULL;
	int *deplacements = NULL;
	int debut_colonnes, nb_colonnes;
	int *L_local = NULL;
	int longueur;
	int erreur_locale, erreur_globale;
	double t0, t1, t2, t3;
	double t_calcul;
	double locaux[5];
	double maxima[5];
	int p;

	MPI_Comm_rank(comm, &rang);
	MPI_Comm_size(comm, &nprocs);

	if (MAITRE == rang) {
		dimensions[0] = m;
		dimensions[1] = n;
	}

	MPI_Barrier(comm);
	t0 = MPI_Wtime();

	/* ---------- Distribution : X entier pour tous, Y decoupe en colonnes ---------- */
	MPI_Bcast(dimensions, 2, MPI_INT, MAITRE, comm);
	m = dimensions[0];
	n = dimensions[1];

	if (MAITRE == rang) {
		X_local = (char *)X;
	} else {
		X_local = malloc((size_t)m + 1);
	}
	MPI_Bcast(X_local, m, MPI_CHAR, MAITRE, comm);

	calculer_segment(n, nprocs, rang, &debut_colonnes, &nb_colonnes);
	Y_local = malloc((size_t)nb_colonnes + 1);

	if (MAITRE == rang) {
		comptes = malloc(nprocs * sizeof(int));
		deplacements = malloc(nprocs * sizeof(int));
		for (p = 0; p < nprocs; p++) {
			calculer_segment(n, nprocs, p, &deplacements[p], &comptes[p]);
		}
	}
	MPI_Scatterv(Y, comptes, deplacements, MPI_CHAR, Y_local, nb_colonnes, MPI_CHAR, MAITRE, comm);

	t1 = MPI_Wtime();

	/* ---------- Phase 1 : matrice des scores ---------- */
	L_local = lcs_phase1_mpi(X_local, Y_local, m, nb_colonnes, hauteur_bloc, rang, nprocs, comm, &t_calcul);

	erreur_locale = (NULL == L_local);
	MPI_Allreduce(&erreur_locale, &erreur_globale, 1, MPI_INT, MPI_MAX, comm); // tout le monde abandonne ensemble
	if (erreur_globale) {
		if (MAITRE == rang) {
			fprintf(stderr, "Erreur : allocation de la matrice locale impossible (m=%d, n=%d, P=%d)\n", m, n, nprocs);
		}
		free(L_local);
		free(Y_local);
		free(comptes);
		free(deplacements);
		if (MAITRE != rang) {
			free(X_local);
		}
		return -1;
	}

	t2 = MPI_Wtime();

	/* ---------- Phase 2 : traceback distribue ---------- */
	longueur = lcs_phase2_mpi(L_local, X_local, Y_local, m, nb_colonnes, rang, nprocs, comm, S);

	t3 = MPI_Wtime();

	locaux[0] = t1 - t0;
	locaux[1] = t2 - t1;
	locaux[2] = t_calcul;
	locaux[3] = t3 - t2;
	locaux[4] = t3 - t0;
	MPI_Reduce(locaux, maxima, 5, MPI_DOUBLE, MPI_MAX, MAITRE, comm); // temps = maximum sur les processus

	if (MAITRE == rang) {
		temps->t_distribution = maxima[0];
		temps->t_phase1 = maxima[1];
		temps->t_calcul_phase1 = maxima[2];
		temps->t_phase2 = maxima[3];
		temps->t_total = maxima[4];
	}

	free(L_local);
	free(Y_local);
	free(comptes);
	free(deplacements);
	if (MAITRE != rang) {
		free(X_local);
	}

	return longueur;
}
