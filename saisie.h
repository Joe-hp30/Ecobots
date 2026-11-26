/*
 * saisie.h — Clavier et souris (partie IHM)
 *
 * Traduit les touches et les clics en t_commande. Le moteur ne voit jamais les codes de touches.
 * Fonctionne sous Windows (console Windows) et sous macOS / Linux (termios + souris xterm),
 * pour que tous les membres de l'équipe puissent compiler.
 */

#ifndef SAISIE_H
#define SAISIE_H

#include "simulation.h"

/* À appeler une fois au début et une fois à la fin du programme */
void saisie_initialiser(void);
void saisie_terminer(void);

/* Lecture NON bloquante : renvoie AUCUNE si rien n'a été tapé ou cliqué.
 * Pour un CLIC, *x et *y reçoivent la colonne et la ligne de l'écran (à partir de 0). */
t_commande lire_commande(int *x, int *y);

/* Lit un choix de menu entre min et max ; attend tant que la saisie est invalide */
int lire_choix_menu(int min, int max);

/* Attend que l'utilisateur appuie sur une touche */
void attendre_touche(void);

/* Met le programme en pause pendant "ms" millisecondes */
void attendre_ms(int ms);

#endif
