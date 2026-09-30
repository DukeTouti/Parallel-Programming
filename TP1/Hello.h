#ifndef HELLO_H
#define HELLO_H

#include <mpi.h>

/*Affiche le rang, le nombre total de processus et la machine hote du processus courant*/
void afficher_hello(int rang, int taille);

#endif /* HELLO_H */
