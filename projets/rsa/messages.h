#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>
#include <stdint.h>

#define ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 "
#define GRAINE 42

/*Genere un message de "longueur" caracteres tires aleatoirement dans ALPHABET, apres
  srand(GRAINE) : le jeu de test regenere exactement le meme message pour verifier.
  Renvoie NULL si l'allocation echoue.*/
char *generer_message(int longueur);

/*Empreinte FNV-1a sur 64 bits de "taille" octets (comparaison sans tout afficher)*/
uint64_t empreinte(const void *donnees, size_t taille);

#endif /* MESSAGES_H */
