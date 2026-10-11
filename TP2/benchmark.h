#ifndef BENCHMARK_H
#define BENCHMARK_H

/*
 * mode : "vecteur", "matvec" ou "matmat"
 * La taille mesuree est celle de la compilation (ARRAY_SIZE ou SIZE)
 * Le rang 0 ajoute une ligne dans fichier_csv, retourne 0 si tout va bien
 */
int benchmark(const char *mode, const char *fichier_csv, int rang, int nb_proc);

/*
 * Lance les 3 programmes avec verification et affichage du prof
 * Retourne le nombre d'echecs
 */
int tests(int rang, int nb_proc);

#endif
