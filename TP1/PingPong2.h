#ifndef PINGPONG2_H
#define PINGPONG2_H

#include <mpi.h>

/*Processus 0 : envoie le tableau au processus 1, recupere sa reponse et affiche le temps de communication*/
void pingpong_emetteur(double valeurs[], int nb_valeurs, int etiquette);

/*Processus 1 : recoit le tableau du processus 0 puis le lui renvoie tel quel*/
void pingpong_recepteur(double valeurs[], int nb_valeurs, int etiquette);

#endif /* PINGPONG2_H */
