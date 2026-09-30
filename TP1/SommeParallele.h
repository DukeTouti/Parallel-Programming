#ifndef SOMMEPARALLELE_H
#define SOMMEPARALLELE_H

#include <mpi.h>

#define ETIQUETTE_SOMME 42

/*Remplit un tableau de n entiers aleatoires (0 a 99)*/
void generer_tableau_entiers(int tab[], int n);

/*Somme sequentielle (aucune communication, calcul complet sur un seul processus)*/
long long somme_sequentielle(int tab[], int n);

/*Calcule le debut et la taille du segment attribue au processus de rang "rang"
  quand on repartit n elements sur nprocs processus (repartition la plus equilibree possible)*/
void calculer_segment(int n, int nprocs, int rang, int *debut, int *taille_segment);

/*Somme parallele : le maitre (rang 0 du communicateur comm) repartit tab[0..n[
  entre tous les processus de comm, chacun somme son segment, puis le maitre
  collecte et additionne les sommes partielles.
  Sur les esclaves (rang != 0), tab peut etre NULL : ils ne connaissent que leur segment.
  "comm" permet de restreindre le calcul a un sous-ensemble des processus lances
  (voir MPI_Comm_split), pour tester plusieurs "nombres de processus" en une
  seule execution mpirun.
  Valeur de retour significative uniquement sur le rang 0 de comm.*/
long long somme_parallele(int tab[], int n, int rang, int nprocs, MPI_Comm comm);

#endif /* SOMMEPARALLELE_H */
