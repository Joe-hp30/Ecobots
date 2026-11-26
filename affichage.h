/*
 * affichage.h — Tous les affichages du programme (partie IHM)
 *
 * Ces fonctions LISENT l'état de la simulation (const) mais ne le modifient jamais.
 */

#ifndef AFFICHAGE_H
#define AFFICHAGE_H

#include "simulation.h"

void affichage_initialiser(void);
void afficher_menu(void);
void afficher_regles(void);
void preparer_ecran_simulation(void);
void afficher_simulation(const t_simulation *sim);
void afficher_bilan(const t_simulation *sim);
void afficher_au_revoir(void);

/* Convertit un clic à l'écran (colonne x, ligne y, à partir de 0) en case de la grille.
 * Renvoie 1 si le clic est sur la grille, 0 sinon. */
int ecran_vers_case(int x, int y, t_position *p);

#endif
