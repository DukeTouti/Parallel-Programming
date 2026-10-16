#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "rsa.h"

/* ---------- Constantes ---------- */

#define NB_TACHES 16
#define MAITRE 0

#define NB_CHAMPS 5
#define CH_N 0
#define CH_E 1
#define CH_D 2
#define CH_EXECUTEE 3 // 1 si la tâche a été exécutée
#define CH_SUCCES 4 // 1 si generate_rsa_keys a renvoyé 0

/* Tableau 1 du sujet : paires (p, q) */
static const long long premiers[NB_TACHES][2] = {
	{101, 113}, {107, 127}, {131, 137}, {139, 149},
	{151, 163}, {167, 173}, {179, 191}, {193, 197},
	{199, 211}, {223, 227}, {229, 233}, {239, 241},
	{251, 257}, {263, 269}, {271, 277}, {281, 283}
};

/* ---------- Calcul parallèle ---------- */

/* Distribution cyclique : la tâche i est traitée par le processus (i % taille) */
void calculer_taches(int rang, int taille, long long local[NB_TACHES][NB_CHAMPS]) {
	for (int i = rang; i < NB_TACHES; i += taille) {
		long long N = 0;
		long long e = 0;
		long long d = 0;
		int retour = generate_rsa_keys(premiers[i][0], premiers[i][1], &N, &e, &d);

		local[i][CH_N] = N;
		local[i][CH_E] = e;
		local[i][CH_D] = d;
		local[i][CH_EXECUTEE] = 1;
		local[i][CH_SUCCES] = (retour == 0);

		printf("[processus %d] tache %2d : p=%lld q=%lld -> N=%lld e=%lld d=%lld\n", rang, i + 1, premiers[i][0], premiers[i][1], N, e, d);
	}
}

/* ---------- Vérification (maître) ---------- */

/* Retourne le nombre d'erreurs détectées */
int verifier_resultats(long long global[NB_TACHES][NB_CHAMPS]) {
	int erreurs = 0;

	for (int i = 0; i < NB_TACHES; i++) {
		long long p = premiers[i][0];
		long long q = premiers[i][1];
		long long phi = (p - 1) * (q - 1);
		long long N = global[i][CH_N];
		long long e = global[i][CH_E];
		long long d = global[i][CH_D];
		long long N_seq = 0;
		long long e_seq = 0;
		long long d_seq = 0;
		int ok = 1;

		if (global[i][CH_EXECUTEE] != 1) { // chaque tâche exactement une fois
			printf("  tache %2d : executee %lld fois (attendu 1)\n", i + 1, global[i][CH_EXECUTEE]);
			ok = 0;
		}
		if (global[i][CH_SUCCES] != 1) {
			printf("  tache %2d : generate_rsa_keys a echoue\n", i + 1);
			ok = 0;
		}
		if (gcd(e, phi) != 1 || (e * d) % phi != 1) { // gcd(e, phi) = 1 et e*d mod phi = 1
			printf("  tache %2d : verification gcd / e*d mod phi echouee\n", i + 1);
			ok = 0;
		}
		generate_rsa_keys(p, q, &N_seq, &e_seq, &d_seq); // référence séquentielle
		if (N != N_seq || e != e_seq || d != d_seq) {
			printf("  tache %2d : resultat different du sequentiel\n", i + 1);
			ok = 0;
		}
		if (!ok) {
			erreurs++;
		}
	}
	return erreurs;
}

/* ---------- Programme principal ---------- */

int main(int argc, char **argv) {
	int rang;
	int taille;
	long long local[NB_TACHES][NB_CHAMPS] = {{0}};
	long long global[NB_TACHES][NB_CHAMPS] = {{0}};

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	MPI_Comm_size(MPI_COMM_WORLD, &taille);

	calculer_taches(rang, taille, local);

	MPI_Reduce(&local[0][0], &global[0][0], NB_TACHES * NB_CHAMPS,
		MPI_LONG_LONG, MPI_SUM, MAITRE, MPI_COMM_WORLD); // une ligne n'est remplie que par un processus

	if (rang == MAITRE) {
		printf("\n===== Resultats collectes (%d processus) =====\n", taille);
		printf("%-6s %-5s %-5s %-8s %-6s %-8s\n", "Tache", "p", "q", "N", "e", "d");
		for (int i = 0; i < NB_TACHES; i++) {
			printf("%-6d %-5lld %-5lld %-8lld %-6lld %-8lld\n", i + 1,
				premiers[i][0], premiers[i][1],
				global[i][CH_N], global[i][CH_E], global[i][CH_D]);
		}

		int erreurs = verifier_resultats(global);
		if (erreurs == 0) {
			printf("\nVerification OK : %d cles valides, identiques au sequentiel.\n", NB_TACHES);
		} else {
			printf("\nVerification KO : %d tache(s) en erreur.\n", erreurs);
		}
	}

	MPI_Finalize();
	return 0;
}
