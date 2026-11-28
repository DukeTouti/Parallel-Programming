#ifndef SEQUENCES_H
#define SEQUENCES_H

#define ALPHABET "ACGT"
#define GRAINE 42

/*Genere X (m symboles) puis Y (n symboles) tires aleatoirement dans ALPHABET, apres
  srand(GRAINE) : le jeu de test regenere exactement les memes sequences pour verifier.
  Renvoie 1 si les allocations ont reussi, 0 sinon.*/
int generer_sequences(int m, int n, char **X, char **Y);

#endif /* SEQUENCES_H */
