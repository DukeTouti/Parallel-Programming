#ifndef RSA_MPI_H
#define RSA_MPI_H

#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include "arithmetique.h"

#define MAITRE 0
#define ETIQUETTE_LIGNE 10    // maitre -> esclave : numero de la ligne a peupler
#define ETIQUETTE_FIN 11      // maitre -> esclave : plus de ligne, l'esclave s'arrete
#define ETIQUETTE_RESULTAT 12 // esclave -> maitre : ligne peuplee

#define LARGEUR_LIGNE 1024 // nombre d'impairs par ligne de la table

/* Indice de la case (i, j) d'une matrice stockee ligne par ligne de "largeur" colonnes */
#define IDX(i, j, largeur) ((size_t)(i) * (size_t)(largeur) + (size_t)(j))

/* Repartition des blocs du message (et des caracteres correspondants) entre les processus */
typedef struct {
	int *comptes_blocs;
	int *deplacements_blocs;
	int *comptes_caracteres;
	int *deplacements_caracteres;
} repartition_t;

/* Temps d'une execution parallele (en secondes, maximum sur les processus) */
typedef struct {
	double t_communication; // t_total - calcul pur : echanges, attentes et desequilibre
	double t_premiers;      // table des impairs, maitre / esclaves, extraction des premiers
	double t_cles;          // tirage de p et q, r, s, diffusion de la cle
	double t_chiffrement;   // Scatterv du texte, C = B^r mod m, Gatherv des blocs chiffres
	double t_dechiffrement; // Scatterv des blocs chiffres, B = C^s mod m, Gatherv du texte
	double t_total;         // de la distribution des parametres au texte dechiffre
} temps_mpi_t;

/*Calcule le debut et la taille du segment attribue au processus de rang "rang"
  quand on repartit n elements sur nprocs processus (repartition la plus
  equilibree possible : les n % nprocs premiers recoivent un element de plus)*/
void calculer_segment(int n, int nprocs, int rang, int *debut, int *taille_segment);

/*Repartit les NB_BLOCS(n, k) blocs du message par segments contigus et en deduit les
  caracteres de chaque processus (k par bloc, moins pour le dernier). Tous les processus
  la calculent. Renvoie 1 si les allocations ont reussi, 0 sinon.*/
int calculer_repartition(int n, int k, int nprocs, repartition_t *repartition);

/*Libere les tableaux de la repartition*/
void liberer_repartition(repartition_t *repartition);

/*Phase "premiers" (pseudo-code parallele du sujet) : le maitre initialise la ligne 0 de la
  table des impairs et la diffuse, puis distribue dynamiquement les nb_lignes lignes aux
  esclaves (une ligne a la fois, une nouvelle ligne des qu'un esclave rend la sienne).
  Chaque esclave peuple ses lignes (le nombre s'il est premier, 0 sinon). Avec un seul
  processus, le maitre peuple lui-meme toutes les lignes. "table" (nb_lignes x
  LARGEUR_LIGNE, maitre uniquement) recoit en tete les premiers trouves, dans l'ordre
  croissant. Renvoie leur nombre sur le maitre (0 ailleurs).
  *t_calcul est augmente du temps de calcul pur du processus.*/
int phase_premiers_mpi(uint32_t premier_impair, int nb_lignes, int rang, int nprocs,
	MPI_Comm comm, uint32_t *table, double *t_calcul);

/*Phase "chiffrement" : repartition du texte (maitre) par MPI_Scatterv, chaque processus
  chiffre ses blocs (C = B^r mod m), puis rassemblement dans "chiffre" (maitre) par
  MPI_Gatherv, dans l'ordre d'origine*/
void chiffrer_mpi(const char *texte, const cle_rsa_t *cle, const repartition_t *repartition,
	int rang, MPI_Comm comm, uint64_t *chiffre, double *t_calcul);

/*Phase "dechiffrement" : repartition des blocs chiffres, B = C^s mod m et reconstitution
  des caracteres par chaque processus, rassemblement du texte dans "dechiffre" (maitre)*/
void dechiffrer_mpi(const uint64_t *chiffre, const cle_rsa_t *cle, const repartition_t *repartition,
	int rang, MPI_Comm comm, char *dechiffre, double *t_calcul);

/*Algorithme complet. premier_impair, nb_lignes, texte et n ne sont significatifs que sur
  le maitre : diffusion des parametres, phase premiers, cles (maitre puis MPI_Bcast),
  chiffrement, dechiffrement. Sur le maitre, *cle recoit la cle, *premiers les premiers
  trouves, *chiffre les blocs chiffres et *dechiffre le texte dechiffre (a liberer), et
  *temps les temps. Renvoie le nombre de premiers sur le maitre, -1 en cas d'erreur.*/
int rsa_mpi(uint32_t premier_impair, int nb_lignes, const char *texte, int n, MPI_Comm comm,
	cle_rsa_t *cle, uint32_t **premiers, uint64_t **chiffre, char **dechiffre, temps_mpi_t *temps);

#endif /* RSA_MPI_H */
