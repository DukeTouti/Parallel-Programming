#include <stdio.h>
#include <stdlib.h>
#include "lcs_cuda.cuh"

/* ---------- Informations sur le GPU ---------- */

void afficher_infos_gpu(void) {
	cudaDeviceProp proprietes;
	int peripherique;

	VERIFIER_CUDA(cudaGetDevice(&peripherique));
	VERIFIER_CUDA(cudaGetDeviceProperties(&proprietes, peripherique));

	printf("GPU : %s | capacite de calcul %d.%d | %d SM | memoire globale %.0f Mo | memoire partagee/bloc %zu Ko\n",
		proprietes.name, proprietes.major, proprietes.minor, proprietes.multiProcessorCount,
		proprietes.totalGlobalMem / (1024.0 * 1024.0), proprietes.sharedMemPerBlock / 1024);
}

/* ---------- Phase 1, methode 1 : une anti-diagonale de cellules par lancement ---------- */

/*Les cellules (i, j) telles que i + j = diagonale sont independantes : elles ne dependent
  que des anti-diagonales diagonale-1 et diagonale-2, calculees par les lancements
  precedents (les lancements d'un meme flux s'executent dans l'ordre).
  Le thread k calcule la cellule i = i_min + k, j = diagonale - i.*/
__global__ void noyau_diagonale(int *L, const char *X, const char *Y, int n,
	int diagonale, int i_min, int longueur_diagonale) {
	int k = blockIdx.x * blockDim.x + threadIdx.x;
	int largeur = n + 1;
	int i, j;

	if (k < longueur_diagonale) {
		i = i_min + k;
		j = diagonale - i;

		if (X[i - 1] == Y[j - 1]) {
			L[IDX(i, j, largeur)] = L[IDX(i - 1, j - 1, largeur)] + 1;
		} else {
			L[IDX(i, j, largeur)] = MAXIMUM(L[IDX(i - 1, j, largeur)], L[IDX(i, j - 1, largeur)]);
		}
	}
}

/*Un lancement de noyau_diagonale par anti-diagonale : m + n - 1 lancements*/
static void phase1_diagonales(int *dL, const char *dX, const char *dY, int m, int n, int threads_par_bloc) {
	int diagonale;

	for (diagonale = 2; diagonale <= m + n; diagonale++) {
		int i_min = MAXIMUM(1, diagonale - n);
		int i_max = (diagonale - 1 < m) ? diagonale - 1 : m;
		int longueur_diagonale = i_max - i_min + 1;
		int nb_blocs = (longueur_diagonale + threads_par_bloc - 1) / threads_par_bloc;

		noyau_diagonale<<<nb_blocs, threads_par_bloc>>>(dL, dX, dY, n, diagonale, i_min, longueur_diagonale);
	}
}

/* ---------- Phase 1, methode 2 : front d'onde de tuiles en memoire partagee ---------- */

/*Un bloc de T threads calcule une tuile T x T de L ; les tuiles d'une meme anti-diagonale
  de tuiles sont independantes et calculees par les blocs d'un meme lancement.
  Dans la tuile, le thread t traite la ligne t : a l'etape k il calcule la colonne k - t
  (front d'onde interne, 2T - 1 etapes separees par __syncthreads()).
  La tuile et sa bordure (ligne du dessus, colonne de gauche) sont en memoire partagee,
  dans un tableau de T + 1 lignes et LARGEUR = T + 2 colonnes : deux threads voisins
  accedent a des mots distants de LARGEUR - 1 = T + 1 (cases (t, k-t) et (t+1, k-t-1)),
  nombre premier avec 32, donc un warp touche 32 bancs differents (aucun conflit). Avec
  LARGEUR = T + 1, la distance serait T = 32 : conflit a 32 voies, acces serialises.
  Les cases hors de la matrice (tuiles du bord bas / droit) recoivent des valeurs neutres :
  elles ne sont jamais recopiees et aucune case valide n'en depend (une case ne depend que
  des cases situees au-dessus et a gauche).*/
__global__ void noyau_tuiles(int *L, const char *X, const char *Y, int m, int n,
	int diagonale_tuiles, int tuile_i_min, int T) {
	extern __shared__ int memoire_partagee[];
	int largeur_smem = T + 2;
	int *s = memoire_partagee;
	char *sx = (char *)&memoire_partagee[(T + 1) * largeur_smem];
	char *sy = sx + T;
	int largeur = n + 1;
	int t = threadIdx.x;
	int tuile_i = tuile_i_min + blockIdx.x;
	int tuile_j = diagonale_tuiles - tuile_i;
	int i0 = tuile_i * T + 1; // premiere ligne de la tuile dans L
	int j0 = tuile_j * T + 1; // premiere colonne de la tuile dans L
	int k, c, r;

	/* ----- Chargement de la bordure et des symboles ----- */
	s[t + 1] = (j0 + t <= n) ? L[IDX(i0 - 1, j0 + t, largeur)] : 0; // ligne du dessus (coalescent)
	s[(t + 1) * largeur_smem] = (i0 + t <= m) ? L[IDX(i0 + t, j0 - 1, largeur)] : 0; // colonne de gauche
	sx[t] = (i0 + t <= m) ? X[i0 + t - 1] : '\0';
	sy[t] = (j0 + t <= n) ? Y[j0 + t - 1] : '\1';
	if (0 == t) {
		s[0] = L[IDX(i0 - 1, j0 - 1, largeur)]; // coin superieur gauche
	}
	__syncthreads();

	/* ----- Front d'onde interne a la tuile ----- */
	for (k = 0; k < 2 * T - 1; k++) {
		c = k - t;
		if (c >= 0 && c < T) {
			if (sx[t] == sy[c]) {
				s[(t + 1) * largeur_smem + (c + 1)] = s[t * largeur_smem + c] + 1;
			} else {
				s[(t + 1) * largeur_smem + (c + 1)] = MAXIMUM(s[t * largeur_smem + (c + 1)], s[(t + 1) * largeur_smem + c]);
			}
		}
		__syncthreads();
	}

	/* ----- Ecriture de la tuile dans L ----- */
	for (r = 0; r < T; r++) {
		if (i0 + r <= m && j0 + t <= n) { // les T threads ecrivent T cases consecutives (coalescent)
			L[IDX(i0 + r, j0 + t, largeur)] = s[(r + 1) * largeur_smem + (t + 1)];
		}
	}
}

/*Un lancement de noyau_tuiles par anti-diagonale de tuiles : ceil(m/T) + ceil(n/T) - 1 lancements*/
static void phase1_tuiles(int *dL, const char *dX, const char *dY, int m, int n, int T) {
	int nb_tuiles_i = (m + T - 1) / T;
	int nb_tuiles_j = (n + T - 1) / T;
	size_t taille_smem = (size_t)(T + 1) * (T + 2) * sizeof(int) + 2 * (size_t)T * sizeof(char);
	int diagonale_tuiles;

	for (diagonale_tuiles = 0; diagonale_tuiles <= nb_tuiles_i + nb_tuiles_j - 2; diagonale_tuiles++) {
		int tuile_i_min = MAXIMUM(0, diagonale_tuiles - (nb_tuiles_j - 1));
		int tuile_i_max = (diagonale_tuiles < nb_tuiles_i - 1) ? diagonale_tuiles : nb_tuiles_i - 1;
		int nb_blocs = tuile_i_max - tuile_i_min + 1;

		noyau_tuiles<<<nb_blocs, T, taille_smem>>>(dL, dX, dY, m, n, diagonale_tuiles, tuile_i_min, T);
	}
}

/* ---------- Phase 2 sur GPU : traceback par un seul thread ---------- */

/*La remontee est intrinsequement sequentielle (chaque pas depend du precedent) : un seul
  thread la realise directement dans la memoire du GPU, ce qui evite de rapatrier les
  (m+1)(n+1) entiers de L. Seule la LCS (au plus min(m, n) caracteres) traverse le bus PCIe.*/
__global__ void noyau_traceback(const int *L, const char *X, const char *Y, int m, int n, char *S) {
	int largeur = n + 1;
	int k = L[IDX(m, n, largeur)];
	int i = m;
	int j = n;

	while (i > 0 && j > 0) {
		if (X[i - 1] == Y[j - 1]) {
			k--;
			S[k] = X[i - 1];
			i--;
			j--;
		} else if (L[IDX(i - 1, j, largeur)] >= L[IDX(i, j - 1, largeur)]) {
			i--;
		} else {
			j--;
		}
	}
}

/* ---------- Algorithme complet ---------- */

/*Allocation, transfert de X et Y, mise a zero des bords (ligne 0 par cudaMemset, colonne 0
  par cudaMemset2D : le reste de L est entierement ecrit par la phase 1), phase 1, phase 2.
  Les temps sont mesures par evenements CUDA, sur le GPU lui-meme.*/
int lcs_cuda(const char *X, const char *Y, int m, int n, int methode, int T, char **S, temps_cuda_t *temps) {
	size_t largeur = (size_t)n + 1;
	size_t taille_matrice = (size_t)(m + 1) * largeur * sizeof(int);
	int *dL = NULL;
	char *dX = NULL;
	char *dY = NULL;
	char *dS = NULL;
	int longueur = 0;
	cudaEvent_t e_debut, e_transfert, e_phase1, e_phase2;
	float ms_transfert, ms_phase1, ms_phase2;

	/* ---------- Allocation ---------- */
	if (cudaSuccess != cudaMalloc((void **)&dL, taille_matrice)) {
		size_t libre, totale;

		cudaGetLastError(); // efface l'erreur d'allocation, le contexte reste valide
		cudaMemGetInfo(&libre, &totale);
		fprintf(stderr, "Erreur : matrice de %.0f Mo trop grande pour le GPU (%.0f Mo libres sur %.0f Mo)\n",
			taille_matrice / (1024.0 * 1024.0), libre / (1024.0 * 1024.0), totale / (1024.0 * 1024.0));
		return -1;
	}
	VERIFIER_CUDA(cudaMalloc((void **)&dX, (size_t)m + 1));
	VERIFIER_CUDA(cudaMalloc((void **)&dY, (size_t)n + 1));
	VERIFIER_CUDA(cudaMalloc((void **)&dS, (size_t)(m < n ? m : n) + 1));

	VERIFIER_CUDA(cudaEventCreate(&e_debut));
	VERIFIER_CUDA(cudaEventCreate(&e_transfert));
	VERIFIER_CUDA(cudaEventCreate(&e_phase1));
	VERIFIER_CUDA(cudaEventCreate(&e_phase2));

	/* ---------- Transfert des sequences et bords ---------- */
	VERIFIER_CUDA(cudaEventRecord(e_debut));
	VERIFIER_CUDA(cudaMemcpy(dX, X, (size_t)m, cudaMemcpyHostToDevice));
	VERIFIER_CUDA(cudaMemcpy(dY, Y, (size_t)n, cudaMemcpyHostToDevice));
	VERIFIER_CUDA(cudaMemset(dL, 0, largeur * sizeof(int))); // ligne 0
	VERIFIER_CUDA(cudaMemset2D(dL, largeur * sizeof(int), 0, sizeof(int), (size_t)m + 1)); // colonne 0
	VERIFIER_CUDA(cudaEventRecord(e_transfert));

	/* ---------- Phase 1 ---------- */
	if (m > 0 && n > 0) {
		if (METHODE_DIAGONALES == methode) {
			phase1_diagonales(dL, dX, dY, m, n, T);
		} else {
			phase1_tuiles(dL, dX, dY, m, n, T);
		}
		VERIFIER_CUDA(cudaGetLastError());
	}
	VERIFIER_CUDA(cudaEventRecord(e_phase1));

	/* ---------- Phase 2 : traceback sur le GPU, seule la LCS revient sur l'hote ---------- */
	VERIFIER_CUDA(cudaMemcpy(&longueur, &dL[IDX(m, n, largeur)], sizeof(int), cudaMemcpyDeviceToHost));
	*S = (char *)malloc((size_t)longueur + 1);
	(*S)[longueur] = '\0';
	noyau_traceback<<<1, 1>>>(dL, dX, dY, m, n, dS);
	VERIFIER_CUDA(cudaGetLastError());
	VERIFIER_CUDA(cudaMemcpy(*S, dS, (size_t)longueur, cudaMemcpyDeviceToHost));
	VERIFIER_CUDA(cudaEventRecord(e_phase2));
	VERIFIER_CUDA(cudaEventSynchronize(e_phase2));

	VERIFIER_CUDA(cudaEventElapsedTime(&ms_transfert, e_debut, e_transfert));
	VERIFIER_CUDA(cudaEventElapsedTime(&ms_phase1, e_transfert, e_phase1));
	VERIFIER_CUDA(cudaEventElapsedTime(&ms_phase2, e_phase1, e_phase2));
	temps->t_transfert = ms_transfert / 1e3;
	temps->t_phase1 = ms_phase1 / 1e3;
	temps->t_phase2 = ms_phase2 / 1e3;
	temps->t_total = temps->t_transfert + temps->t_phase1 + temps->t_phase2;

	VERIFIER_CUDA(cudaEventDestroy(e_debut));
	VERIFIER_CUDA(cudaEventDestroy(e_transfert));
	VERIFIER_CUDA(cudaEventDestroy(e_phase1));
	VERIFIER_CUDA(cudaEventDestroy(e_phase2));
	VERIFIER_CUDA(cudaFree(dL));
	VERIFIER_CUDA(cudaFree(dX));
	VERIFIER_CUDA(cudaFree(dY));
	VERIFIER_CUDA(cudaFree(dS));

	return longueur;
}
