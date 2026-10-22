#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>
#include "rsa_text_mpi.h"

/* Usage : mpirun -np P ./rsa_text_mpi [message|@N] [std|alea] [repetitions] [csv]
   - message     : texte à chiffrer (défaut : "Challenge")
   - @N          : génère un texte de N caractères (utile pour les benchmarks,
                   car la ligne de commande est limitée à ~128 Ko par argument)
   - std | alea  : version standard (défaut) ou variante avec aléa (bonus)
   - repetitions : nombre de passes mesurées (après une passe à blanc) ; on
                   affiche la médiane des temps (défaut : 1)
   - csv         : n'affiche qu'une ligne CSV (pour bench.sh) */

#define MAITRE 0
#define MESSAGE_DEFAUT "Challenge"
#define AFFICHAGE_MAX 64 // au-delà, on n'affiche pas toutes les valeurs chiffrées

/* Paire (p, q) du tableau du sujet */
#define P_PREMIER 281
#define Q_PREMIER 283

typedef struct {
	int rang;
	int taille;
	int n;
	int alea;
	long long N;
	long long e;
	long long d;
	unsigned int graine;
	const char *message; // significatif chez le processus 0 uniquement
	int *comptes;
	int *deplacements;
	char *local_texte;
	long long *local_chiffre;
	char *local_decode;
	long long *chiffre; // significatifs chez le processus 0 uniquement
	char *decode;
} contexte_t;

typedef struct {
	double enc_total; // scatter + calcul + gather (vu par le processus 0)
	double enc_calc;  // calcul seul (maximum sur les processus)
	double dec_total;
	double dec_calc;
} temps_t;

/* ---------- Répartition par blocs ----------
   Les caractères sont répartis en blocs contigus : les (n % taille) premiers
   processus reçoivent un caractère de plus. Si taille > n, certains processus
   reçoivent 0 caractère. */
static void repartition(int n, int taille, int *comptes, int *deplacements) {
	int base = n / taille;
	int reste = n % taille;
	int position = 0;

	for (int i = 0; i < taille; i++) {
		comptes[i] = base + (i < reste ? 1 : 0);
		deplacements[i] = position;
		position += comptes[i];
	}
}

/* ---------- Une passe complète : chiffrement puis déchiffrement parallèles ---------- */
static void passe(contexte_t *c, temps_t *t) {
	int nb_local = c->comptes[c->rang];

	/* ===== Chiffrement ===== */
	MPI_Barrier(MPI_COMM_WORLD);
	double t0 = MPI_Wtime();

	MPI_Scatterv(c->message, c->comptes, c->deplacements, MPI_CHAR,
		c->local_texte, nb_local, MPI_CHAR, MAITRE, MPI_COMM_WORLD);

	double t1 = MPI_Wtime();
	for (int i = 0; i < nb_local; i++) {
		if (c->alea) {
			c->local_chiffre[i] = chiffrer_caractere_alea(c->local_texte[i], c->N, c->e, &c->graine);
		} else {
			c->local_chiffre[i] = chiffrer_caractere(c->local_texte[i], c->N, c->e);
		}
	}
	double t2 = MPI_Wtime();

	MPI_Gatherv(c->local_chiffre, nb_local, MPI_LONG_LONG,
		c->chiffre, c->comptes, c->deplacements, MPI_LONG_LONG, MAITRE, MPI_COMM_WORLD);

	double t3 = MPI_Wtime();
	double calcul = t2 - t1;
	double calcul_max = 0;
	MPI_Reduce(&calcul, &calcul_max, 1, MPI_DOUBLE, MPI_MAX, MAITRE, MPI_COMM_WORLD);
	t->enc_total = t3 - t0;
	t->enc_calc = calcul_max;

	/* ===== Déchiffrement ===== */
	MPI_Barrier(MPI_COMM_WORLD);
	double t4 = MPI_Wtime();

	MPI_Scatterv(c->chiffre, c->comptes, c->deplacements, MPI_LONG_LONG,
		c->local_chiffre, nb_local, MPI_LONG_LONG, MAITRE, MPI_COMM_WORLD);

	double t5 = MPI_Wtime();
	for (int i = 0; i < nb_local; i++) {
		if (c->alea) {
			c->local_decode[i] = dechiffrer_caractere_alea(c->local_chiffre[i], c->N, c->d);
		} else {
			c->local_decode[i] = dechiffrer_caractere(c->local_chiffre[i], c->N, c->d);
		}
	}
	double t6 = MPI_Wtime();

	MPI_Gatherv(c->local_decode, nb_local, MPI_CHAR,
		c->decode, c->comptes, c->deplacements, MPI_CHAR, MAITRE, MPI_COMM_WORLD);

	double t7 = MPI_Wtime();
	calcul = t6 - t5;
	calcul_max = 0;
	MPI_Reduce(&calcul, &calcul_max, 1, MPI_DOUBLE, MPI_MAX, MAITRE, MPI_COMM_WORLD);
	t->dec_total = t7 - t4;
	t->dec_calc = calcul_max;
}

/* ---------- Médiane ---------- */
static int comparer_double(const void *a, const void *b) {
	double x = *(const double *)a;
	double y = *(const double *)b;

	return (x > y) - (x < y);
}

static double mediane(double *valeurs, int nb) {
	qsort(valeurs, nb, sizeof(double), comparer_double);
	if (nb % 2 == 1) {
		return valeurs[nb / 2];
	}
	return (valeurs[nb / 2 - 1] + valeurs[nb / 2]) / 2;
}

/* ---------- Programme principal ---------- */

int main(int argc, char **argv) {
	int rang;
	int taille;
	int statut = 0;
	const char *message = MESSAGE_DEFAUT;
	char *message_genere = NULL;
	int params[4] = {0, 0, 1, 0}; // longueur (-1 si erreur), alea, repetitions, csv
	long long cles[4] = {0, 0, 0, 0}; // N, e, d, graine de base

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	MPI_Comm_size(MPI_COMM_WORLD, &taille);

	/* ----- Le processus 0 définit le texte et génère les clés ----- */
	if (rang == MAITRE) {
		if (argc > 1) {
			message = argv[1];
		}

		if (message[0] == '@') { // texte généré de N caractères imprimables
			int n_gen = atoi(message + 1);
			if (n_gen > 0) {
				message_genere = malloc(n_gen + 1);
				for (int i = 0; i < n_gen; i++) {
					message_genere[i] = (char)(32 + (i * 7) % 95);
				}
				message_genere[n_gen] = '\0';
				message = message_genere;
			} else {
				message = "";
			}
		}

		params[0] = (int)strlen(message);
		params[1] = (argc > 2 && strcmp(argv[2], "alea") == 0);
		params[2] = (argc > 3) ? atoi(argv[3]) : 1;
		if (params[2] < 1) {
			params[2] = 1;
		}
		params[3] = (argc > 4 && strcmp(argv[4], "csv") == 0);

		if (params[0] == 0) {
			printf("Erreur : le message est vide\n");
			params[0] = -1;
		} else if (generate_rsa_keys(P_PREMIER, Q_PREMIER, &cles[0], &cles[1], &cles[2]) != 0) {
			printf("Erreur : generation des cles impossible\n");
			params[0] = -1;
		} else if (cles[0] <= 255) {
			printf("Erreur : N trop petit pour chiffrer des caracteres\n");
			params[0] = -1;
		}
		cles[3] = (long long)time(NULL);
	}

	MPI_Bcast(params, 4, MPI_INT, MAITRE, MPI_COMM_WORLD);
	if (params[0] < 0) {
		free(message_genere);
		MPI_Finalize();
		return 1;
	}
	MPI_Bcast(cles, 4, MPI_LONG_LONG, MAITRE, MPI_COMM_WORLD);

	int n = params[0];
	int reps = params[2];
	int csv = params[3];

	contexte_t c;
	c.rang = rang;
	c.taille = taille;
	c.n = n;
	c.alea = params[1];
	c.N = cles[0];
	c.e = cles[1];
	c.d = cles[2];
	c.graine = (unsigned int)cles[3] + (unsigned int)rang; // graine différente par processus
	c.message = (rang == MAITRE) ? message : NULL;

	c.comptes = malloc(taille * sizeof(int));
	c.deplacements = malloc(taille * sizeof(int));
	repartition(n, taille, c.comptes, c.deplacements);

	int nb_local = c.comptes[rang];
	c.local_texte = malloc(nb_local + 1);
	c.local_chiffre = malloc((nb_local + 1) * sizeof(long long));
	c.local_decode = malloc(nb_local + 1);
	c.chiffre = NULL;
	c.decode = NULL;
	if (rang == MAITRE) {
		c.chiffre = malloc(n * sizeof(long long));
		c.decode = malloc(n + 1);
	}

	/* ----- Passe à blanc (initialisation de MPI, caches), puis passes mesurées ----- */
	temps_t t;
	double *v_enc_total = malloc(reps * sizeof(double));
	double *v_enc_calc = malloc(reps * sizeof(double));
	double *v_dec_total = malloc(reps * sizeof(double));
	double *v_dec_calc = malloc(reps * sizeof(double));

	passe(&c, &t);
	for (int r = 0; r < reps; r++) {
		passe(&c, &t);
		v_enc_total[r] = t.enc_total;
		v_enc_calc[r] = t.enc_calc;
		v_dec_total[r] = t.dec_total;
		v_dec_calc[r] = t.dec_calc;
	}

	/* ===== Affichage et vérification (processus 0) ===== */
	if (rang == MAITRE) {
		c.decode[n] = '\0';
		int ok = (memcmp(message, c.decode, n) == 0);

		double m_enc_total = mediane(v_enc_total, reps) * 1000; // en ms
		double m_enc_calc = mediane(v_enc_calc, reps) * 1000;
		double m_dec_total = mediane(v_dec_total, reps) * 1000;
		double m_dec_calc = mediane(v_dec_calc, reps) * 1000;

		if (!ok) {
			statut = 1;
		}

		if (csv) {
			printf("%d,%d,%s,%d,%.6f,%.6f,%.6f,%.6f\n", taille, n, c.alea ? "alea" : "std", reps,
				m_enc_total, m_enc_calc, m_dec_total, m_dec_calc);
			if (!ok) {
				fprintf(stderr, "Verification KO (np=%d, n=%d)\n", taille, n);
			}
		} else {
			printf("===== RSA texte parallele (%d processus, %s) =====\n", taille, c.alea ? "variante alea" : "version standard");
			printf("Cles : N=%lld e=%lld d=%lld\n", c.N, c.e, c.d);
			printf("Longueur du message : %d caractere(s)\n\n", n);

			printf("Repartition :\n");
			for (int i = 0; i < taille; i++) {
				printf("  processus %d : %d caractere(s)", i, c.comptes[i]);
				if (c.comptes[i] > 0) {
					printf(" [%d..%d]", c.deplacements[i], c.deplacements[i] + c.comptes[i] - 1);
				}
				printf("\n");
			}

			if (n <= AFFICHAGE_MAX) {
				printf("\nTexte original : %s\n", message);
				printf("Valeurs chiffrees :");
				for (int i = 0; i < n; i++) {
					printf(" %lld", c.chiffre[i]);
				}
				printf("\nTexte dechiffre : %s\n", c.decode);
			} else {
				printf("\n(message trop long : affichage des valeurs omis)\n");
			}

			if (ok) {
				printf("\nVerification OK : le texte dechiffre est identique a l'original.\n");
			} else {
				printf("\nVerification KO : le texte dechiffre differe de l'original.\n");
			}

			printf("\nTemps (ms), mediane sur %d repetition(s) apres une passe a blanc :\n", reps);
			printf("  chiffrement   : total = %.4f | calcul (max processus) = %.4f | communication ~ %.4f\n",
				m_enc_total, m_enc_calc, m_enc_total - m_enc_calc);
			printf("  dechiffrement : total = %.4f | calcul (max processus) = %.4f | communication ~ %.4f\n",
				m_dec_total, m_dec_calc, m_dec_total - m_dec_calc);
		}
	}

	free(v_enc_total);
	free(v_enc_calc);
	free(v_dec_total);
	free(v_dec_calc);
	free(c.comptes);
	free(c.deplacements);
	free(c.local_texte);
	free(c.local_chiffre);
	free(c.local_decode);
	free(c.chiffre);
	free(c.decode);
	free(message_genere);

	MPI_Finalize();
	return statut;
}
