#ifndef PINGPONG1_H
#define PINGPONG1_H

#include <mpi.h>

/*Processus 0 : envoie le tableau de valeurs au processus 1 (ping simple)*/
void ping_emetteur(double valeurs[], int nb_valeurs, int etiquette);

/*Processus 1 : recoit le tableau de valeurs et affiche la derniere*/
void ping_recepteur(double valeurs[], int nb_valeurs, int etiquette);

#endif /* PINGPONG1_H */
