#include <stddef.h>
#include "rsa.h"

/* Algorithme d'Euclide */
long long gcd(long long a, long long b) {
	while (b != 0) {
		long long r = a % b;
		a = b;
		b = r;
	}

	return a;
}

/* Algorithme d'Euclide étendu */
long long extended_gcd(long long a, long long b, long long *x, long long *y) {
	if (b == 0) {
		*x = 1;
		*y = 0;
		return a;
	}

	long long x1, y1;

	long long g = extended_gcd(b, a % b, &x1, &y1);

	*x = y1;
	*y = x1 - (a / b) * y1;

	return g;
}

/* Inverse modulaire */
long long modular_inverse(long long e, long long phi) {
	long long x, y;

	long long g = extended_gcd(e, phi, &x, &y);

	if (g != 1) {
		return -1;
	}

	return (x % phi + phi) % phi;
}

/* Teste si n est premier */
int is_prime(long long n) {
	if (n < 2) {
		return 0;
	}

	if (n == 2) {
		return 1;
	}

	if (n % 2 == 0) {
		return 0;
	}

	for (long long i = 3; i <= n / i; i += 2) {
		if (n % i == 0) {
			return 0;
		}
	}

	return 1;
}

/* Génère les clés RSA
   Retourne 0 en cas de succès et -1 en cas d'échec. */
int generate_rsa_keys(long long p, long long q, long long *N, long long *e, long long *d) {
	if (N == NULL || e == NULL || d == NULL) {
		return -1;
	}

	if (!is_prime(p) || !is_prime(q) || p == q) {
		return -1;
	}

	*N = p * q;

	long long phi = (p - 1) * (q - 1);

	*e = 3;

	while (*e < phi && gcd(*e, phi) != 1) {
		*e += 2;
	}

	if (*e >= phi) {
		return -1;
	}

	*d = modular_inverse(*e, phi);

	if (*d == -1) {
		return -1;
	}

	if (gcd(*e, phi) != 1 || ((*e * *d) % phi) != 1) {
		return -1;
	}

	return 0;
}
