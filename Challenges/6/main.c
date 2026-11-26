/*
 * Challenge 6 - Factorisation : Fermat vs recherche classique de période (Shor)
 * Version C séquentielle des deux scripts Python.
 *
 * Usage : ./bench [maxIterFermat] [maxPeriod]
 *   ex.   ./bench 1000000000 1000000000
 *
 * Fichiers produits :
 *   resultats_fermat.csv : Fermat, N croissant
 *   resultats_shor.csv   : période, exp 1 (N croissant, a = 2),
 *                          exp 2 (N = 10403, a variable),
 *                          exp 3 (autres bases, question 5)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef unsigned long long u64;

#define REPS     3        /* nombre de mesures, on garde la meilleure      */
#define LONG_RUN 1.0      /* au-delà d'1 s, une seule mesure suffit        */
#define NONE     0xFFFFFFFFFFFFFFFFULL

/* ==========================================================
 * Arithmétique
 * ========================================================== */

/* Racine carrée entière exacte : sqrt() en double peut se tromper d'une
 * unité au-delà de 2^53, on corrige donc le résultat. */
static u64 isqrt_u64(u64 n)
{
	u64 r = (u64)sqrt((double)n);
	if (r > 0xFFFFFFFFULL) r = 0xFFFFFFFFULL;
	while (r * r > n) r--;
	while (r < 0xFFFFFFFFULL && (r + 1) * (r + 1) <= n) r++;
	return r;
}

/* (a * b) mod m sans débordement sur 64 bits.
 * m < 2^32 : le produit tient sur 64 bits.
 * sinon     : quotient estimé en double puis corrigé (valable pour m < 2^50). */
static u64 mulmod(u64 a, u64 b, u64 m)
{
	if (m < 0x100000000ULL)
		return (a * b) % m;
	u64 q = (u64)((double)a * (double)b / (double)m);
	long long r = (long long)(a * b - q * m);
	while (r < 0)             r += (long long)m;
	while (r >= (long long)m) r -= (long long)m;
	return (u64)r;
}

static u64 powmod(u64 a, u64 e, u64 m)
{
	u64 res = 1 % m;
	a %= m;
	while (e) {
		if (e & 1) res = mulmod(res, a, m);
		a = mulmod(a, a, m);
		e >>= 1;
	}
	return res;
}

static u64 gcd_u64(u64 a, u64 b)
{
	while (b) { u64 t = a % b; a = b; b = t; }
	return a;
}

static double now_s(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * 1e-9;
}

/* ==========================================================
 * Fermat
 * ========================================================== */

/* Renvoie le nombre d'itérations (0 si échec), p et q */
static u64 fermat_factor(u64 N, u64 maxIter, u64 *p, u64 *q)
{
	*p = *q = 0;
	if (N % 2 == 0) { *p = 2; *q = N / 2; return 0; }

	u64 x = isqrt_u64(N);
	if (x * x < N) x++;

	for (u64 it = 1; it <= maxIter; it++, x++) {
		u64 y2 = x * x - N;
		u64 y  = isqrt_u64(y2);
		if (y * y == y2) {
			*p = x - y;
			*q = x + y;
			return it;
		}
	}
	return 0;
}

/* ==========================================================
 * Recherche classique de période (Shor)
 * ========================================================== */

/* Renvoie le statut ; r = période (NONE si non trouvée),
 * iters = nombre de multiplications modulaires effectuées */
static const char *shor_factor(u64 N, u64 a, u64 maxPeriod,
                               u64 *p, u64 *q, u64 *r, u64 *iters)
{
	*p = *q = 0;
	*r = NONE;
	*iters = 0;

	/* Étape 1 : pgcd(a, N) */
	u64 d = gcd_u64(a, N);
	if (d > 1 && d < N) { *p = d; *q = N / d; return "GCD"; }
	if (d != 1)         return "INVALID";

	/* Étape 2 : plus petit r tel que a^r = 1 (mod N) */
	u64 v = 1;
	for (u64 k = 1; k <= maxPeriod; k++) {
		v = mulmod(v, a, N);
		(*iters)++;
		if (v == 1) { *r = k; break; }
	}
	if (*r == NONE) return "NOT_FOUND";

	/* Étape 3 : r doit être pair */
	if (*r % 2) return "ODD_PERIOD";

	/* Étapes 4-5 : x = a^(r/2) mod N, éviter x = -1 */
	u64 x = powmod(a, *r / 2, N);
	if (x == N - 1) return "TRIVIAL";

	/* Étapes 6-7 : facteurs par pgcd, vérification */
	u64 g1 = gcd_u64(x - 1, N), g2 = gcd_u64(x + 1, N);
	if (g1 > 1 && g1 < N && g2 > 1 && g2 < N && g1 * g2 == N) {
		*p = g1; *q = g2;
		return "OK";
	}
	return "FAIL";
}

/* ==========================================================
 * Cas de test
 * ========================================================== */

/* Fermat : cas du TP (facteurs proches puis éloignés),
 * puis entiers plus grands (question 5) */
static const u64 F_P[] = {
	101ULL, 1009ULL, 10007ULL, 100003ULL, 1000003ULL,
	100003ULL, 100003ULL, 100003ULL, 100003ULL, 100003ULL,
	1000003ULL, 1000003ULL, 1000003ULL, 1000000007ULL
};
static const u64 F_Q[] = {
	103ULL, 1013ULL, 10037ULL, 100019ULL, 1000033ULL,
	110017ULL, 150001ULL, 200003ULL, 500009ULL, 1000003ULL,
	10000019ULL, 100000007ULL, 1000000007ULL, 1000000009ULL
};
#define NB_F (int)(sizeof(F_P) / sizeof(F_P[0]))

/* Période : {expérience, N, a} */
static const u64 S[][3] = {
	/* Exp 1 : mêmes N que Fermat (cas du TP), a = 2 */
	{1, 10403ULL, 2}, {1, 1022117ULL, 2}, {1, 100440259ULL, 2},
	{1, 10002200057ULL, 2}, {1, 1000036000099ULL, 2}, {1, 11002030051ULL, 2},
	{1, 15000550003ULL, 2}, {1, 20000900009ULL, 2}, {1, 50002400027ULL, 2},
	{1, 100003300009ULL, 2},
	/* Exp 2 : N = 10403 fixé, base variable */
	{2, 10403ULL, 2}, {2, 10403ULL, 3}, {2, 10403ULL, 5}, {2, 10403ULL, 7},
	{2, 10403ULL, 11}, {2, 10403ULL, 13}, {2, 10403ULL, 17}, {2, 10403ULL, 19},
	{2, 10403ULL, 23},
	/* Exp 3 : autres bases sur des N plus grands (question 5) */
	{3, 1022117ULL, 7}, {3, 100440259ULL, 3}, {3, 11002030051ULL, 3},
	{3, 15000550003ULL, 3}, {3, 20000900009ULL, 7}
};
#define NB_S (int)(sizeof(S) / sizeof(S[0]))

/* Ligne de séparation pour l'affichage */
static void sep(char c)
{
	for (int i = 0; i < 90; i++) putchar(c);
	putchar('\n');
}

/* ==========================================================
 * Programme principal
 * ========================================================== */

int main(int argc, char **argv)
{
	u64 maxIter   = (argc > 1) ? strtoull(argv[1], NULL, 10) : 1000000000ULL;
	u64 maxPeriod = (argc > 2) ? strtoull(argv[2], NULL, 10) : 1000000000ULL;

	/* ---------------- Fermat ---------------- */
	FILE *f = fopen("resultats_fermat.csv", "w");
	if (!f) { perror("fopen"); return 1; }
	fprintf(f, "test,N,p,q,iterations,time_s,ok\n");

	sep('=');
	printf("FERMAT FACTORIZATION - PERFORMANCE EVALUATION\n");
	sep('=');
	printf("%-22s %-12s %-12s %-12s %-15s\n", "N", "Factor p", "Factor q", "Iterations", "Time (seconds)");
	sep('-');

	for (int t = 0; t < NB_F; t++) {
		u64 N = F_P[t] * F_Q[t], p, q, it = 0;
		double best = 1e30;
		for (int k = 0; k < REPS; k++) {
			double t0 = now_s();
			it = fermat_factor(N, maxIter, &p, &q);
			double dt = now_s() - t0;
			if (dt < best) best = dt;
			if (dt > LONG_RUN) break;
		}
		int ok = (it > 0 && p * q == N);
		printf("%-22llu %-12llu %-12llu %-12llu %.9f%s\n",
		       N, p, q, it, best, ok ? "" : "  ECHEC");
		fprintf(f, "%d,%llu,%llu,%llu,%llu,%.9f,%d\n", t + 1, N, p, q, it, best, ok);
	}
	fclose(f);

	/* ---------------- Période ---------------- */
	f = fopen("resultats_shor.csv", "w");
	if (!f) { perror("fopen"); return 1; }
	fprintf(f, "exp,test,N,a,p,q,r,iterations,status,time_s,ok\n");

	const char *titres[] = { "",
		"EXPERIMENT 1 : INCREASING N (a = 2)",
		"EXPERIMENT 2 : INFLUENCE OF THE BASE a (N = 10403)",
		"EXPERIMENT 3 : OTHER BASES ON LARGER N" };
	int exp_courante = 0;

	for (int t = 0; t < NB_S; t++) {
		int e = (int)S[t][0];
		u64 N = S[t][1], a = S[t][2];

		if (e != exp_courante) {
			exp_courante = e;
			putchar('\n'); sep('=');
			printf("CLASSICAL SHOR-INSPIRED FACTORIZATION - %s\n", titres[e]);
			sep('=');
			printf("%-16s %-5s %-10s %-10s %-12s %-12s %-11s %-15s\n",
			       "N", "a", "Factor p", "Factor q", "Period r", "Iterations", "Status", "Time (seconds)");
			sep('-');
		}

		u64 p, q, r, iters;
		const char *st = "";
		double best = 1e30;
		for (int k = 0; k < REPS; k++) {
			double t0 = now_s();
			st = shor_factor(N, a, maxPeriod, &p, &q, &r, &iters);
			double dt = now_s() - t0;
			if (dt < best) best = dt;
			if (dt > LONG_RUN) break;
		}
		long long rr = (r == NONE) ? -1LL : (long long)r;
		int ok = (p != 0);

		printf("%-16llu %-5llu %-10llu %-10llu %-12lld %-12llu %-11s %.9f\n",
		       N, a, p, q, rr, iters, st, best);
		fprintf(f, "%d,%d,%llu,%llu,%llu,%llu,%lld,%llu,%s,%.9f,%d\n",
		        e, t + 1, N, a, p, q, rr, iters, st, best, ok);
	}
	fclose(f);

	printf("\nRésultats : resultats_fermat.csv, resultats_shor.csv\n");
	return 0;
}
