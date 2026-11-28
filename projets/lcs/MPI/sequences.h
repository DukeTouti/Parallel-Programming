#ifndef SEQUENCES_H
#define SEQUENCES_H

#define ALPHABET "ACGT"

/*Lit une sequence dans un fichier texte (les blancs et retours a la ligne sont ignores).
  La longueur lue est ecrite dans *longueur. Renvoie NULL en cas d'erreur.*/
char *lire_sequence(const char *chemin, int *longueur);

/*Alloue une sequence de "longueur" symboles tires aleatoirement dans ALPHABET*/
char *generer_sequence(int longueur);

/*Renvoie 1 si la chaine ne contient que des chiffres (argument = taille), 0 sinon*/
int est_entier(const char *chaine);

#endif /* SEQUENCES_H */
