#include <stdlib.h>
#include "arithmetique.h"

/* ---------- Arithmetique ---------- */

/*Algorithme d'Euclide : pgcd(a, b) = pgcd(b, a mod b), le dernier reste non nul est le pgcd*/
uint64_t pgcd(uint64_t a, uint64_t b) {
	uint64_t reste;

	while (0 != b) {
		reste = a % b;
		a = b;
		b = reste;
	}

	return a;
}

/*Algorithme d'Euclide etendu (forme iterative) : a chaque etape, reste = coef * r (mod phi).
  Quand le reste precedent vaut pgcd(r, phi) = 1, le coefficient associe est l'inverse de r.
  Les coefficients de Bezout peuvent depasser 2^63 en valeur absolue (phi < 2^64) :
  ils sont calcules sur 128 bits signes.*/
uint64_t inverse_modulaire(uint64_t r, uint64_t phi) {
	__int128 reste_precedent = phi;
	__int128 reste = r;
	__int128 coef_precedent = 0;
	__int128 coef = 1;
	__int128 quotient, tampon;

	while (0 != reste) {
		quotient = reste_precedent / reste;

		tampon = reste_precedent - quotient * reste;
		reste_precedent = reste;
		reste = tampon;

		tampon = coef_precedent - quotient * coef;
		coef_precedent = coef;
		coef = tampon;
	}

	if (1 != reste_precedent) { // r et phi ne sont pas premiers entre eux
		return 0;
	}
	if (coef_precedent < 0) {
		coef_precedent += phi;
	}

	return (uint64_t)coef_precedent;
}

/*m < 2^64 : le produit a * b tient sur 128 bits, le reste sur 64 bits*/
uint64_t mul_modulaire(uint64_t a, uint64_t b, uint64_t m) {
	return (uint64_t)(((unsigned __int128)a * b) % m);
}

/*Version iterative de Expmod : a^y = (a^2)^(y/2) si y est pair, a * (a^2)^((y-1)/2) sinon.
  On parcourt les bits de y du poids faible au poids fort : a prend les valeurs a, a^2,
  a^4, ... et on multiplie le resultat par a^(2^i) quand le bit i de y vaut 1.
  log2(y) iterations, au plus 2 multiplications modulaires chacune.*/
uint64_t exp_modulaire(uint64_t a, uint64_t y, uint64_t m) {
	uint64_t resultat = 1 % m;

	a %= m;

	while (y > 0) {
		if (y & 1) {
			resultat = mul_modulaire(resultat, a, m);
		}
		a = mul_modulaire(a, a, m);
		y >>= 1;
	}

	return resultat;
}

/*Divisions par 2 puis par les impairs 3, 5, 7, ... tant que i <= racine(n) (i <= n / i
  evite le debordement de i * i). Au pire racine(n) / 2 divisions.*/
int est_premier(uint32_t n) {
	uint32_t i;

	if (n < 2) {
		return 0;
	}
	if (2 == n) {
		return 1;
	}
	if (0 == n % 2) {
		return 0;
	}

	for (i = 3; i <= n / i; i += 2) {
		if (0 == n % i) {
			return 0;
		}
	}

	return 1;
}

/* ---------- Table des impairs et cles ---------- */

/*Premier impair de la table, 0 si la table sort de [1, 2^32 - 1]*/
uint32_t premier_impair_table(uint64_t borne, int nb_nombres) {
	uint64_t premier = borne | 1; // borne si elle est impaire, borne + 1 sinon
	uint64_t dernier;

	if (nb_nombres < 1) {
		return 0;
	}

	dernier = premier + 2 * (uint64_t)(nb_nombres - 1);
	if (dernier > UINT32_MAX) {
		return 0;
	}

	return (uint32_t)premier;
}

/*256^(k+1) <= m equivaut a 256^k <= m / 256 (division entiere) : pas de debordement.
  m < 2^64 donc k <= 7.*/
int taille_bloc(uint64_t m) {
	uint64_t puissance = 256; // 256^k
	int k = 1;

	while (puissance <= m / 256) {
		puissance *= 256;
		k++;
	}

	return k;
}

/*Creation de la cle comme dans le sujet (p, q, m, phi, r, s)*/
int generer_cles(const uint32_t *premiers, int nb_premiers, cle_rsa_t *cle) {
	int i, j;

	if (nb_premiers < 2) {
		return 0;
	}

	/* ---------- p et q distincts tires au hasard dans la table ---------- */
	srand(GRAINE_CLES);
	i = rand() % nb_premiers;
	do {
		j = rand() % nb_premiers;
	} while (j == i);

	cle->p = premiers[i];
	cle->q = premiers[j];
	cle->m = (uint64_t)cle->p * cle->q;
	cle->phi = (uint64_t)(cle->p - 1) * (cle->q - 1);

	/* ---------- r premier avec phi, 1 < r < phi ---------- */
	cle->r = R_INITIAL;
	while (cle->r < cle->phi && 1 != pgcd(cle->r, cle->phi)) {
		cle->r += 2;
	}
	if (cle->r >= cle->phi || cle->m < 256) {
		return 0;
	}

	/* ---------- s = r^-1 mod phi (Euclide etendu) ---------- */
	cle->s = inverse_modulaire(cle->r, cle->phi);
	cle->k = taille_bloc(cle->m);

	return (0 != cle->s && 1 == mul_modulaire(cle->r, cle->s, cle->phi));
}

/* ---------- Blocs ---------- */

/*B = c0 * 256^(k-1) + c1 * 256^(k-2) + ... + c(k-1), les caracteres absents valant 0*/
uint64_t empaqueter_bloc(const char *texte, int nb_caracteres, int k) {
	uint64_t bloc = 0;
	int i;

	for (i = 0; i < k; i++) {
		bloc <<= 8;
		if (i < nb_caracteres) {
			bloc |= (unsigned char)texte[i];
		}
	}

	return bloc;
}

/*Le caractere i est l'octet de poids 256^(k-1-i) du bloc*/
void depaqueter_bloc(uint64_t bloc, char *texte, int nb_caracteres, int k) {
	int i;

	for (i = 0; i < nb_caracteres; i++) {
		texte[i] = (char)((bloc >> (8 * (k - 1 - i))) & 0xFF);
	}
}
