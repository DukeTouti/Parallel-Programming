#ifndef RING_H
#define RING_H

#define MESSAGE_INITIAL 10
#define ETIQUETTE_RING 201

/*Calcule les rangs du processus suivant et precedent dans l'anneau*/
void calculer_voisins(int rang, int taille, int *suivant, int *precedent);

/*Le processus 0 initialise le message et l'envoie au processus suivant*/
void envoyer_message_initial(int suivant, int message, int etiquette);

/*Fait circuler le message dans l'anneau jusqu'a ce qu'il atteigne 0*/
void faire_circuler_message(int rang, int suivant, int precedent, int etiquette);

/*Le processus 0 recoit le dernier passage du message pour cloturer le tour*/
void recevoir_message_final(int precedent, int etiquette);

#endif /* RING_H */
