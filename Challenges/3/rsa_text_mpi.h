#ifndef RSA_TEXT_MPI_H
#define RSA_TEXT_MPI_H

#include "rsa.h"

/* Exponentiation modulaire : (base^exp) mod mod */
long long mod_pow(long long base, long long exp, long long mod);

/* Chiffre un caractère avec la clé publique (N, e) */
long long chiffrer_caractere(char c, long long N, long long e);

/* Déchiffre une valeur chiffrée avec la clé privée (N, d) */
char dechiffrer_caractere(long long c, long long N, long long d);

/* Variante avec aléa (bonus) : même caractère -> chiffrés différents.
   graine : état de rand_r, à initialiser différemment par processus. */
long long chiffrer_caractere_alea(char c, long long N, long long e, unsigned int *graine);
char dechiffrer_caractere_alea(long long c, long long N, long long d);

#endif
