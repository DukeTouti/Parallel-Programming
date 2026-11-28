#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "jeu_de_test.h"
#include "sequentiel.h"

/* Configurations testees pour chaque taille */
static const int nprocs_a_tester[] = {1, 2, 4, 8};
static const char *methodes_cuda[] = {"tuiles", "diagonales"};
static const int parametres_cuda[] = {32, 256}; // cote de la tuile | threads par bloc

#define NB_NPROCS ((int)(sizeof(nprocs_a_tester) / sizeof(nprocs_a_tester[0])))
#define NB_METHODES_CUDA ((int)(sizeof(methodes_cuda) / sizeof(methodes_cuda[0])))

/* ---------- Outils ---------- */

/*Renvoie un instant en secondes (horloge monotone)*/
static double chrono(void) {
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * 1e-9;
}

/*Compare deux doubles pour qsort*/
static int comparer_doubles(const void *a, const void *b) {
	double da = *(const double *)a;
	double db = *(const double *)b;

	return (da > db) - (da < db);
}

/*Mediane d'un tableau de nb doubles (le tableau est trie)*/
static double mediane(double valeurs[], int nb) {
	qsort(valeurs, nb, sizeof(double), comparer_doubles);
	if (nb % 2 == 1) {
		return valeurs[nb / 2];
	}
	return (valeurs[nb / 2 - 1] + valeurs[nb / 2]) / 2.0;
}

/*Ouvre le CSV (ecrase) et ecrit la ligne d'en-tete*/
FILE *ouvrir_csv(void) {
	FILE *csv = fopen(FICHIER_CSV, "w");

	if (NULL == csv) {
		perror(FICHIER_CSV);
		return NULL;
	}

	fprintf(csv, "version,configuration,taille,nb_processus,t_seq,t_communication,t_phase1,t_phase2,t_total,acceleration,longueur_lcs,correct\n");
	return csv;
}

/* ---------- Sequences ---------- */

/*Meme generation que MPI/sequences.c et CUDA/sequences.cu : srand(GRAINE), X puis Y*/
static char *generer_sequence(int longueur) {
	char *sequence = malloc((size_t)longueur + 1);
	int taille_alphabet = (int)strlen(ALPHABET);
	int i;

	for (i = 0; i < longueur; i++) {
		sequence[i] = ALPHABET[rand() % taille_alphabet];
	}
	sequence[longueur] = '\0';

	return sequence;
}

/* ---------- Executions ---------- */

/*Lance une commande (MPI ou CUDA en mode csv) et lit sa ligne de resultat :
  longueur,t_communication,t_phase1,t_phase2,t_total,LCS
  La LCS est renvoyee dans *S (allouee). Renvoie 1 si tout s'est bien passe.*/
static int executer(const char *commande, resultat_t *r, char **S) {
	FILE *sortie = popen(commande, "r");
	char *ligne = NULL;
	size_t taille_ligne = 0;
	ssize_t lus;
	int debut_lcs = 0;
	int ok = 0;

	if (NULL == sortie) {
		perror(commande);
		return 0;
	}

	lus = getline(&ligne, &taille_ligne, sortie);
	if (lus > 0 && 5 == sscanf(ligne, "%d,%lf,%lf,%lf,%lf,%n", &r->longueur, &r->t_communication,
			&r->t_phase1, &r->t_phase2, &r->t_total, &debut_lcs) && debut_lcs > 0) {
		ligne[strcspn(ligne, "\n")] = '\0'; // retire le retour a la ligne
		*S = strdup(&ligne[debut_lcs]);
		ok = 1;
	}

	free(ligne);
	if (0 != pclose(sortie)) {
		ok = 0;
	}
	if (!ok) {
		fprintf(stderr, "Echec de la commande : %s\n", commande);
	}
	return ok;
}

/*Execute NB_REPETITIONS fois la commande, garde la mediane de chaque temps et verifie
  la LCS de la premiere execution contre la reference sequentielle. Renvoie 1 si correct.*/
static int executer_et_verifier(const char *commande, const char *X, const char *Y, int n,
	const char *S_ref, int longueur_ref, resultat_t *med) {
	double t_communication[NB_REPETITIONS], t_phase1[NB_REPETITIONS], t_phase2[NB_REPETITIONS], t_total[NB_REPETITIONS];
	int correct = 0;
	int rep;

	memset(med, 0, sizeof(*med)); // en cas d'echec, la ligne sortira en ECHEC avec des zeros

	for (rep = 0; rep < NB_REPETITIONS; rep++) {
		resultat_t r;
		char *S = NULL;

		if (!executer(commande, &r, &S)) {
			return 0;
		}

		if (0 == rep) { // verification sur la premiere execution
			correct = (r.longueur == longueur_ref)
				&& ((int)strlen(S) == r.longueur)
				&& (0 == strcmp(S, S_ref))
				&& est_sous_sequence(S, r.longueur, X, n)
				&& est_sous_sequence(S, r.longueur, Y, n);
			med->longueur = r.longueur;
		}

		t_communication[rep] = r.t_communication;
		t_phase1[rep] = r.t_phase1;
		t_phase2[rep] = r.t_phase2;
		t_total[rep] = r.t_total;
		free(S);
	}

	med->t_communication = mediane(t_communication, NB_REPETITIONS);
	med->t_phase1 = mediane(t_phase1, NB_REPETITIONS);
	med->t_phase2 = mediane(t_phase2, NB_REPETITIONS);
	med->t_total = mediane(t_total, NB_REPETITIONS);
	return correct;
}

/*Affiche une ligne de resultat et l'ajoute au CSV*/
static void enregistrer(FILE *csv, const char *version, const char *configuration, int n, int nprocs,
	double t_seq, const resultat_t *r, int correct) {
	double acceleration = (r->t_total > 0.0) ? t_seq / r->t_total : 0.0;

	printf("n = %6d | %-4s %-14s | seq = %9.5f s | par = %9.5f s (ph1 %.5f, ph2 %.5f, comm %.5f) | acc = %7.2fx | %s\n",
		n, version, configuration, t_seq, r->t_total, r->t_phase1, r->t_phase2, r->t_communication,
		acceleration, correct ? "OK" : "ECHEC");

	if (NULL != csv) {
		fprintf(csv, "%s,%s,%d,%d,%.9f,%.9f,%.9f,%.9f,%.9f,%.6f,%d,%d\n", version, configuration, n, nprocs,
			t_seq, r->t_communication, r->t_phase1, r->t_phase2, r->t_total, acceleration, r->longueur, correct);
		fflush(csv);
	}
}

/* ---------- Test d'une taille ---------- */

void tester_taille(int n, FILE *csv, int *nb_tests, int *nb_echecs) {
	char *X, *Y;
	char *S_ref;
	int longueur_ref = 0;
	double essais_seq[NB_REPETITIONS];
	double t_seq;
	char commande[256];
	char configuration[64];
	int rep, p, c;

	/* ---------- Memes sequences que MPI et CUDA ---------- */
	srand(GRAINE);
	X = generer_sequence(n);
	Y = generer_sequence(n);

	/* ---------- Reference sequentielle (code du sujet) ---------- */
	S_ref = malloc((size_t)n + 1);
	for (rep = 0; rep < NB_REPETITIONS; rep++) {
		double debut = chrono();
		int *L = lcs_phase1(X, Y, n, n);

		if (NULL == L) {
			fprintf(stderr, "Erreur : allocation de la matrice sequentielle impossible (n = %d)\n", n);
			free(X);
			free(Y);
			free(S_ref);
			return;
		}
		longueur_ref = lcs_phase2(L, X, Y, n, n, S_ref);
		essais_seq[rep] = chrono() - debut;
		free(L);
	}
	t_seq = mediane(essais_seq, NB_REPETITIONS);

	printf("\n=== n = %d | |LCS| = %d | sequentiel = %.5f s ===\n", n, longueur_ref, t_seq);

	/* ---------- MPI : plusieurs nombres de processus ---------- */
	for (p = 0; p < NB_NPROCS; p++) {
		resultat_t r;
		int correct;

		snprintf(commande, sizeof(commande), "%s -np %d %s %d %d %d csv",
			MPIRUN, nprocs_a_tester[p], EXEC_MPI, n, n, HAUTEUR_BLOC);
		snprintf(configuration, sizeof(configuration), "P=%d", nprocs_a_tester[p]);

		correct = executer_et_verifier(commande, X, Y, n, S_ref, longueur_ref, &r);
		enregistrer(csv, "MPI", configuration, n, nprocs_a_tester[p], t_seq, &r, correct);
		(*nb_tests)++;
		if (!correct) {
			(*nb_echecs)++;
		}
	}

	/* ---------- CUDA : plusieurs methodes ---------- */
	for (c = 0; c < NB_METHODES_CUDA; c++) {
		resultat_t r;
		int correct;

		snprintf(commande, sizeof(commande), "%s %d %d %s %d csv",
			EXEC_CUDA, n, n, methodes_cuda[c], parametres_cuda[c]);
		snprintf(configuration, sizeof(configuration), "%s %d", methodes_cuda[c], parametres_cuda[c]);

		correct = executer_et_verifier(commande, X, Y, n, S_ref, longueur_ref, &r);
		enregistrer(csv, "CUDA", configuration, n, 0, t_seq, &r, correct);
		(*nb_tests)++;
		if (!correct) {
			(*nb_echecs)++;
		}
	}

	free(X);
	free(Y);
	free(S_ref);
}
