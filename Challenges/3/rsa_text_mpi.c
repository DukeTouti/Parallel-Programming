#include <stdlib.h>
#include "rsa_text_mpi.h"

/* ========== Partie I : chiffrement / déchiffrement RSA de texte ========== */

/* Exponentiation modulaire rapide : retourne (base^exp) mod mod.
   Les produits restent dans un long long tant que mod < 2^31 environ,
   ce qui est largement le cas avec les paires (p, q) du sujet. */
long long mod_pow(long long base, long long exp, long long mod) {
	long long resultat = 1;

	base %= mod;

	while (exp > 0) {
		if (exp & 1) {
			resultat = (resultat * base) % mod;
		}
		base = (base * base) % mod;
		exp >>= 1;
	}

	return resultat;
}

/* Chiffre un caractère avec la clé publique (N, e) : c = m^e mod N.
   Le caractère est converti en unsigned char pour obtenir une valeur
   entière dans [0, 255], même pour les caractères "négatifs" (octets >= 128). */
long long chiffrer_caractere(char c, long long N, long long e) {
	long long m = (unsigned char)c;

	return mod_pow(m, e, N);
}

/* Déchiffre une valeur chiffrée avec la clé privée (N, d) : m = c^d mod N,
   puis reconvertit l'entier obtenu en caractère. */
char dechiffrer_caractere(long long c, long long N, long long d) {
	long long m = mod_pow(c, d, N);

	return (char)(unsigned char)m;
}

/* ---------- Variante avec aléa (bonus, version "jouet" du padding) ----------
   On construit m' = r * 256 + m avec r aléatoire, de sorte que m' < N.
   Deux occurrences du même caractère donnent (en général) des chiffrés
   différents. Ce n'est PAS un vrai OAEP : l'aléa est minuscule et N est
   beaucoup trop petit pour être sûr. Nécessite N >= 256. */
long long chiffrer_caractere_alea(char c, long long N, long long e, unsigned int *graine) {
	long long r_max = (N - 256) / 256;
	long long r = rand_r(graine) % (r_max + 1);
	long long m = r * 256 + (unsigned char)c;

	return mod_pow(m, e, N);
}

/* On déchiffre m' puis on jette r (m' mod 256) pour retrouver le caractère. */
char dechiffrer_caractere_alea(long long c, long long N, long long d) {
	long long m = mod_pow(c, d, N);

	return (char)(unsigned char)(m % 256);
}
