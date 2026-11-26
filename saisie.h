/*
 * saisie.h — Lecture du clavier (partie IHM)
 *
 * Traduit les touches en t_commande. Le moteur ne voit jamais les codes de touches.
 * Fonctionne sous Windows (conio.h, comme demandé dans le sujet) et sous macOS / Linux
 * (termios), pour que tous les membres de l'équipe puissent compiler.
 */

#ifndef SAISIE_H
#define SAISIE_H

#include "simulation.h"

/* À appeler une fois au début et une fois à la fin du programme */
void saisie_initialiser(void);
void saisie_terminer(void);

/* Attend une touche et renvoie la commande correspondante (AUCUNE si touche inutile) */
t_commande lire_commande(void);

/* Lit un choix de menu entre min et max ; redemande tant que la saisie est invalide */
int lire_choix_menu(int min, int max);

/* Attend que l'utilisateur appuie sur une touche */
void attendre_touche(void);

#endif
