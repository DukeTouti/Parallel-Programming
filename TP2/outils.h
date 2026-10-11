#ifndef OUTILS_H
#define OUTILS_H

#include <stdio.h>

/* Retourne (sur le rang 0) le maximum des temps locaux de tous les processus */
double chronometre_max(double temps_local);

/* Ouvre le CSV en ajout et ecrit l'en-tete si le fichier est vide */
FILE *ouvrir_csv(const char *chemin);

#endif
