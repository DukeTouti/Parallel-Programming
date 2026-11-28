#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "rsa_mpi.h"

/* ---------- Repartition ---------- */

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

/*Le processus p recoit les blocs [debut, debut + taille[, donc les caracteres
  [debut * k, min((debut + taille) * k, n)[ : seul le dernier bloc peut etre incomplet.
  Un processus sans bloc (plus de processus que de blocs) n'a aucun caractere.*/
int calculer_repartition(int n, int k, int nprocs, repartition_t *repartition) {
	int nb_blocs = NB_BLOCS(n, k);
	int debut, taille_segment;
	int p;

	repartition->comptes_blocs = malloc(nprocs * sizeof(int));
	repartition->deplacements_blocs = malloc(nprocs * sizeof(int));
	repartition->comptes_caracteres = malloc(nprocs * sizeof(int));
	repartition->deplacements_caracteres = malloc(nprocs * sizeof(int));

	if (NULL == repartition->comptes_blocs || NULL == repartition->deplacements_blocs
		|| NULL == repartition->comptes_caracteres || NULL == repartition->deplacements_caracteres) {
		liberer_repartition(repartition);
		return 0;
	}

	for (p = 0; p < nprocs; p++) {
		calculer_segment(nb_blocs, nprocs, p, &debut, &taille_segment);
		repartition->comptes_blocs[p] = taille_segment;
		repartition->deplacements_blocs[p] = debut;
		repartition->deplacements_caracteres[p] = MINIMUM(debut * k, n);
		repartition->comptes_caracteres[p] = MINIMUM((debut + taille_segment) * k, n) - repartition->deplacements_caracteres[p];
	}

	return 1;
}

/*Libere les tableaux de la repartition*/
void liberer_repartition(repartition_t *repartition) {
	free(repartition->comptes_blocs);
	free(repartition->deplacements_blocs);
	free(repartition->comptes_caracteres);
	free(repartition->deplacements_caracteres);
	repartition->comptes_blocs = NULL;
	repartition->deplacements_blocs = NULL;
	repartition->comptes_caracteres = NULL;
	repartition->deplacements_caracteres = NULL;
}

/* ---------- Phase premiers : maitre / esclaves ---------- */

/*Peuple une ligne de la table : la case c de la ligne "ligne" contient l'impair
  ligne0[c] + 2 * LARGEUR_LIGNE * ligne, remplace par 0 s'il n'est pas premier*/
static void peupler_ligne(const uint32_t *ligne0, int ligne, uint32_t *resultat) {
	uint32_t decalage = 2 * LARGEUR_LIGNE * (uint32_t)ligne;
	int c;

	for (c = 0; c < LARGEUR_LIGNE; c++) {
		uint32_t valeur = ligne0[c] + decalage;

		resultat[c] = est_premier(valeur) ? valeur : 0;
	}
}

/*Envoie la prochaine ligne non peuplee a l'esclave, ou le message de fin s'il n'en reste
  plus. ligne_de[esclave] memorise la ligne confiee : le maitre recevra le resultat
  directement a sa place dans la table, sans tampon intermediaire.*/
static void envoyer_travail(int esclave, int nb_lignes, int *prochaine, int *ligne_de, int *actifs, MPI_Comm comm) {
	int fin = -1;

	if (*prochaine < nb_lignes) {
		MPI_Send(prochaine, 1, MPI_INT, esclave, ETIQUETTE_LIGNE, comm);
		ligne_de[esclave] = *prochaine;
		(*prochaine)++;
		(*actifs)++;
	} else {
		MPI_Send(&fin, 1, MPI_INT, esclave, ETIQUETTE_FIN, comm);
	}
}

/*Maitre : une ligne a chaque esclave, puis tant qu'il reste des lignes non peuplees,
  une nouvelle ligne a chaque esclave qui rend la sienne (equilibrage dynamique : le cout
  d'une ligne depend du nombre de premiers qu'elle contient)*/
static void maitre_premiers(int nb_lignes, int nprocs, MPI_Comm comm, uint32_t *table) {
	int *ligne_de = malloc(nprocs * sizeof(int)); // ligne en cours de calcul chez chaque esclave
	int prochaine = 0;
	int actifs = 0;
	int esclave;
	MPI_Status statut;

	for (esclave = 1; esclave < nprocs; esclave++) {
		envoyer_travail(esclave, nb_lignes, &prochaine, ligne_de, &actifs, comm);
	}

	while (actifs > 0) {
		MPI_Probe(MPI_ANY_SOURCE, ETIQUETTE_RESULTAT, comm, &statut); // premier esclave qui a fini
		esclave = statut.MPI_SOURCE;
		MPI_Recv(&table[IDX(ligne_de[esclave], 0, LARGEUR_LIGNE)], LARGEUR_LIGNE, MPI_UINT32_T,
			esclave, ETIQUETTE_RESULTAT, comm, MPI_STATUS_IGNORE);
		actifs--;
		envoyer_travail(esclave, nb_lignes, &prochaine, ligne_de, &actifs, comm);
	}

	free(ligne_de);
}

/*Esclave : peuple chaque ligne recue et la rend au maitre, jusqu'au message de fin*/
static void esclave_premiers(const uint32_t *ligne0, MPI_Comm comm, double *t_calcul) {
	uint32_t resultat[LARGEUR_LIGNE];
	MPI_Status statut;
	int ligne;
	double debut;

	MPI_Recv(&ligne, 1, MPI_INT, MAITRE, MPI_ANY_TAG, comm, &statut);

	while (ETIQUETTE_LIGNE == statut.MPI_TAG) {
		debut = MPI_Wtime();
		peupler_ligne(ligne0, ligne, resultat);
		*t_calcul += MPI_Wtime() - debut;

		MPI_Send(resultat, LARGEUR_LIGNE, MPI_UINT32_T, MAITRE, ETIQUETTE_RESULTAT, comm);
		MPI_Recv(&ligne, 1, MPI_INT, MAITRE, MPI_ANY_TAG, comm, &statut);
	}
}

/*Phase premiers : diffusion de la ligne 0, peuplement des lignes, puis extraction des
  premiers par le maitre (compactage sur place, l'ordre croissant est conserve)*/
int phase_premiers_mpi(uint32_t premier_impair, int nb_lignes, int rang, int nprocs,
	MPI_Comm comm, uint32_t *table, double *t_calcul) {
	uint32_t ligne0[LARGEUR_LIGNE];
	int nb_premiers = 0;
	size_t nb_nombres, i;
	int ligne, c;
	double debut;

	/* ---------- Le maitre initialise la ligne 0 et la diffuse ---------- */
	if (MAITRE == rang) {
		for (c = 0; c < LARGEUR_LIGNE; c++) {
			ligne0[c] = premier_impair + 2 * (uint32_t)c;
		}
	}
	MPI_Bcast(ligne0, LARGEUR_LIGNE, MPI_UINT32_T, MAITRE, comm);

	/* ---------- Peuplement des lignes ---------- */
	if (1 == nprocs) { // pas d'esclave : le maitre peuple toute la table
		debut = MPI_Wtime();
		for (ligne = 0; ligne < nb_lignes; ligne++) {
			peupler_ligne(ligne0, ligne, &table[IDX(ligne, 0, LARGEUR_LIGNE)]);
		}
		*t_calcul += MPI_Wtime() - debut;
	} else if (MAITRE == rang) {
		maitre_premiers(nb_lignes, nprocs, comm, table);
	} else {
		esclave_premiers(ligne0, comm, t_calcul);
	}

	/* ---------- Extraction des premiers (maitre) ---------- */
	if (MAITRE == rang) {
		debut = MPI_Wtime();
		nb_nombres = IDX(nb_lignes, 0, LARGEUR_LIGNE);
		for (i = 0; i < nb_nombres; i++) {
			if (0 != table[i]) {
				table[nb_premiers] = table[i];
				nb_premiers++;
			}
		}
		*t_calcul += MPI_Wtime() - debut;
	}

	return nb_premiers;
}

/* ---------- Phases chiffrement et dechiffrement ---------- */

/*Chaque processus recoit les caracteres de ses blocs et chiffre bloc par bloc*/
void chiffrer_mpi(const char *texte, const cle_rsa_t *cle, const repartition_t *repartition,
	int rang, MPI_Comm comm, uint64_t *chiffre, double *t_calcul) {
	int nb_blocs_local = repartition->comptes_blocs[rang];
	int nb_caracteres_local = repartition->comptes_caracteres[rang];
	char *texte_local = malloc((size_t)nb_caracteres_local + 1);
	uint64_t *chiffre_local = malloc(((size_t)nb_blocs_local + 1) * sizeof(uint64_t));
	int b;
	double debut;

	MPI_Scatterv(texte, repartition->comptes_caracteres, repartition->deplacements_caracteres, MPI_CHAR,
		texte_local, nb_caracteres_local, MPI_CHAR, MAITRE, comm);

	debut = MPI_Wtime();
	for (b = 0; b < nb_blocs_local; b++) {
		int debut_bloc = b * cle->k;
		int nb_caracteres = MINIMUM(cle->k, nb_caracteres_local - debut_bloc);

		chiffre_local[b] = exp_modulaire(empaqueter_bloc(&texte_local[debut_bloc], nb_caracteres, cle->k), cle->r, cle->m);
	}
	*t_calcul += MPI_Wtime() - debut;

	MPI_Gatherv(chiffre_local, nb_blocs_local, MPI_UINT64_T,
		chiffre, repartition->comptes_blocs, repartition->deplacements_blocs, MPI_UINT64_T, MAITRE, comm);

	free(texte_local);
	free(chiffre_local);
}

/*Chaque processus recoit ses blocs chiffres, les dechiffre et reconstitue ses caracteres*/
void dechiffrer_mpi(const uint64_t *chiffre, const cle_rsa_t *cle, const repartition_t *repartition,
	int rang, MPI_Comm comm, char *dechiffre, double *t_calcul) {
	int nb_blocs_local = repartition->comptes_blocs[rang];
	int nb_caracteres_local = repartition->comptes_caracteres[rang];
	char *texte_local = malloc((size_t)nb_caracteres_local + 1);
	uint64_t *chiffre_local = malloc(((size_t)nb_blocs_local + 1) * sizeof(uint64_t));
	int b;
	double debut;

	MPI_Scatterv(chiffre, repartition->comptes_blocs, repartition->deplacements_blocs, MPI_UINT64_T,
		chiffre_local, nb_blocs_local, MPI_UINT64_T, MAITRE, comm);

	debut = MPI_Wtime();
	for (b = 0; b < nb_blocs_local; b++) {
		int debut_bloc = b * cle->k;
		int nb_caracteres = MINIMUM(cle->k, nb_caracteres_local - debut_bloc);

		depaqueter_bloc(exp_modulaire(chiffre_local[b], cle->s, cle->m), &texte_local[debut_bloc], nb_caracteres, cle->k);
	}
	*t_calcul += MPI_Wtime() - debut;

	MPI_Gatherv(texte_local, nb_caracteres_local, MPI_CHAR,
		dechiffre, repartition->comptes_caracteres, repartition->deplacements_caracteres, MPI_CHAR, MAITRE, comm);

	free(texte_local);
	free(chiffre_local);
}

/* ---------- Algorithme complet ---------- */

/*Diffusion des parametres, phases premiers, cles, chiffrement et dechiffrement*/
int rsa_mpi(uint32_t premier_impair, int nb_lignes, const char *texte, int n, MPI_Comm comm,
	cle_rsa_t *cle, uint32_t **premiers, uint64_t **chiffre, char **dechiffre, temps_mpi_t *temps) {
	int rang, nprocs;
	uint64_t parametres[4]; // n, nb_lignes, premier_impair, allocations du maitre reussies
	uint64_t cle_diffusee[8]; // p, q, m, phi, r, s, k, cle valide
	uint32_t *table = NULL;
	repartition_t repartition = {NULL, NULL, NULL, NULL};
	int nb_premiers = 0;
	int erreur_locale, erreur_globale;
	double t0, t1, t2, t3, t4;
	double t_calcul = 0.0;
	double debut;
	double locaux[6];
	double maxima[6];

	MPI_Comm_rank(comm, &rang);
	MPI_Comm_size(comm, &nprocs);

	/* ---------- Allocations du maitre (hors mesure) ---------- */
	if (MAITRE == rang) {
		table = malloc(IDX(nb_lignes, 0, LARGEUR_LIGNE) * sizeof(uint32_t));
		*chiffre = malloc(((size_t)n + 1) * sizeof(uint64_t)); // k >= 1 : au plus n blocs
		*dechiffre = malloc((size_t)n + 1);

		parametres[0] = (uint64_t)n;
		parametres[1] = (uint64_t)nb_lignes;
		parametres[2] = premier_impair;
		parametres[3] = (NULL != table && NULL != *chiffre && NULL != *dechiffre);
	}

	MPI_Barrier(comm);
	t0 = MPI_Wtime();

	/* ---------- Distribution des parametres ---------- */
	MPI_Bcast(parametres, 4, MPI_UINT64_T, MAITRE, comm);
	if (0 == parametres[3]) {
		if (MAITRE == rang) {
			fprintf(stderr, "Erreur : allocation impossible (n = %d, %d lignes)\n", n, nb_lignes);
			free(table);
		}
		return -1;
	}
	n = (int)parametres[0];
	nb_lignes = (int)parametres[1];
	premier_impair = (uint32_t)parametres[2];

	/* ---------- Phase 1 : premiers (maitre / esclaves) ---------- */
	nb_premiers = phase_premiers_mpi(premier_impair, nb_lignes, rang, nprocs, comm, table, &t_calcul);

	t1 = MPI_Wtime();

	/* ---------- Phase 2 : cles (maitre), diffusees a tous ---------- */
	if (MAITRE == rang) {
		debut = MPI_Wtime();
		cle_diffusee[7] = generer_cles(table, nb_premiers, cle);
		t_calcul += MPI_Wtime() - debut;

		cle_diffusee[0] = cle->p;
		cle_diffusee[1] = cle->q;
		cle_diffusee[2] = cle->m;
		cle_diffusee[3] = cle->phi;
		cle_diffusee[4] = cle->r;
		cle_diffusee[5] = cle->s;
		cle_diffusee[6] = (uint64_t)cle->k;
	}
	MPI_Bcast(cle_diffusee, 8, MPI_UINT64_T, MAITRE, comm);

	cle->p = (uint32_t)cle_diffusee[0];
	cle->q = (uint32_t)cle_diffusee[1];
	cle->m = cle_diffusee[2];
	cle->phi = cle_diffusee[3];
	cle->r = cle_diffusee[4];
	cle->s = cle_diffusee[5];
	cle->k = (int)cle_diffusee[6];

	erreur_locale = (0 == cle_diffusee[7]) || !calculer_repartition(n, cle->k, nprocs, &repartition);
	MPI_Allreduce(&erreur_locale, &erreur_globale, 1, MPI_INT, MPI_MAX, comm); // tout le monde abandonne ensemble
	if (erreur_globale) {
		if (MAITRE == rang) {
			fprintf(stderr, "Erreur : generation de la cle impossible (%d premiers trouves)\n", nb_premiers);
			free(table);
		}
		liberer_repartition(&repartition);
		return -1;
	}

	t2 = MPI_Wtime();

	/* ---------- Phase 3 : chiffrement ---------- */
	chiffrer_mpi(texte, cle, &repartition, rang, comm, (MAITRE == rang) ? *chiffre : NULL, &t_calcul);

	t3 = MPI_Wtime();

	/* ---------- Phase 4 : dechiffrement ---------- */
	dechiffrer_mpi((MAITRE == rang) ? *chiffre : NULL, cle, &repartition, rang, comm,
		(MAITRE == rang) ? *dechiffre : NULL, &t_calcul);

	t4 = MPI_Wtime();

	locaux[0] = t1 - t0;
	locaux[1] = t2 - t1;
	locaux[2] = t3 - t2;
	locaux[3] = t4 - t3;
	locaux[4] = t4 - t0;
	locaux[5] = t_calcul;
	MPI_Reduce(locaux, maxima, 6, MPI_DOUBLE, MPI_MAX, MAITRE, comm); // temps = maximum sur les processus

	if (MAITRE == rang) {
		temps->t_premiers = maxima[0];
		temps->t_cles = maxima[1];
		temps->t_chiffrement = maxima[2];
		temps->t_dechiffrement = maxima[3];
		temps->t_total = maxima[4];
		temps->t_communication = maxima[4] - maxima[5];
		(*dechiffre)[n] = '\0';
		*premiers = table; // les nb_premiers premieres cases de la table
	}

	liberer_repartition(&repartition);

	return nb_premiers;
}
