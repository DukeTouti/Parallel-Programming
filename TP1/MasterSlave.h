#ifndef MASTERSLAVE_H
#define MASTERSLAVE_H

#define ETIQUETTE_MASTERSLAVE 10

/*Le maitre (rang 0) lit un entier sur l'entree standard et l'envoie a chaque esclave*/
void maitre(int myrank, int nprocs);

/*Chaque esclave recoit l'entier envoye par le maitre*/
void esclave(int myrank, int master);

#endif /* MASTERSLAVE_H */
