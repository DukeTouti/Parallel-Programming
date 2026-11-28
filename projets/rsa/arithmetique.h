#ifndef ARITHMETIQUE_H
#define ARITHMETIQUE_H

#include <stdint.h>

#define BORNE_MIN 10000 // pseudo-code du sujet : la valeur saisie doit etre > 10000
#define R_INITIAL 65537 // premier exposant de chiffrement essaye (puis r += 2)
#define GRAINE_CLES 2026 // graine du tirage aleatoire de p et q dans la table

/* Nombre de blocs de k caracteres necessaires pour n caracteres (dernier bloc incomplet) */
#define NB_BLOCS(n, k) (((n) + (k) - 1) / (k))

/* min(a, b) */
#define MINIMUM(a, b) (((a) <= (b)) ? (a) : (b))

/* Cle RSA : (m, r) cle publique, (m, s) cle privee */
typedef struct {
	uint32_t p;
	uint32_t q;
	uint64_t m;   // module de chiffrement m = p * q
	uint64_t phi; // indicatrice d'Euler phi(m) = (p - 1) * (q - 1)
	uint64_t r;   // exposant de chiffrement, pgcd(r, phi) = 1
	uint64_t s;   // exposant de dechiffrement, r * s mod phi = 1
	int k;        // nombre de caracteres par bloc (256^k <= m)
} cle_rsa_t;

/*Algorithme d'Euclide : pgcd(a, b)*/
uint64_t pgcd(uint64_t a, uint64_t b);

/*Algorithme d'Euclide etendu : renvoie s tel que r * s mod phi = 1,
  ou 0 si r n'est pas inversible modulo phi*/
uint64_t inverse_modulaire(uint64_t r, uint64_t phi);

/*(a * b) mod m sans debordement (produit sur 128 bits)*/
uint64_t mul_modulaire(uint64_t a, uint64_t b, uint64_t m);

/*a^y mod m par exponentiation rapide (Expmod du sujet, diviser pour regner)*/
uint64_t exp_modulaire(uint64_t a, uint64_t y, uint64_t m);

/*Test de primalite par divisions successives jusqu'a racine(n) (exercice 3 du sujet).
  Renvoie 1 si n est premier, 0 sinon.*/
int est_premier(uint32_t n);

/*Premier impair de la table (borne si elle est impaire, borne + 1 sinon). Renvoie 0 si
  la table de nb_nombres impairs depasse 2^32 - 1 (m = p * q doit tenir sur 64 bits).*/
uint32_t premier_impair_table(uint64_t borne, int nb_nombres);

/*Nombre de caracteres par bloc : plus grand k tel que 256^k <= m (chaque bloc B < m)*/
int taille_bloc(uint64_t m);

/*Tire p et q distincts dans premiers (srand(GRAINE_CLES)), calcule m, phi, r (premier
  impair >= R_INITIAL tel que pgcd(r, phi) = 1) et s = r^-1 mod phi.
  Renvoie 1 si la cle est valide, 0 sinon (moins de deux premiers, r ou s introuvable).*/
int generer_cles(const uint32_t *premiers, int nb_premiers, cle_rsa_t *cle);

/*Code nb_caracteres caracteres (<= k) en un entier B < 256^k, en base 256, poids fort
  en tete ; un bloc incomplet (fin du message) est complete par des zeros*/
uint64_t empaqueter_bloc(const char *texte, int nb_caracteres, int k);

/*Operation inverse : ecrit les nb_caracteres premiers caracteres du bloc dans texte*/
void depaqueter_bloc(uint64_t bloc, char *texte, int nb_caracteres, int k);

#endif /* ARITHMETIQUE_H */
