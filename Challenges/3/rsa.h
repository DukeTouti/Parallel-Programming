#ifndef RSA_H
#define RSA_H

/* Euclidean algorithm */
long long gcd(long long a, long long b);

/* Extended Euclidean algorithm */
long long extended_gcd(long long a, long long b,
                       long long *x, long long *y);

/* Modular inverse */
long long modular_inverse(long long e, long long phi);

/* Check whether n is prime */
int is_prime(long long n);

/*
 * Generate RSA keys from two distinct prime numbers.
 *
 * Returns:
 *   0  : success
 *  -1  : failure
 */
int generate_rsa_keys(long long p, long long q, long long *N, long long *e, long long *d);

#endif
