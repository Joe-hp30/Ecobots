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
void afficher_simulation(const t_simulation *sim);
void afficher_bilan(const t_simulation *sim);
void afficher_au_revoir(void);

#endif
