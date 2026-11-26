/*
 * affichage_raylib.h — Affichage graphique avec raylib (partie IHM, mode graphique)
 *
 * Même rôle que affichage.h, mais dans une fenêtre graphique au lieu de la console.
 * Ces fonctions LISENT l'état de la simulation (const) mais ne le modifient jamais.
 */

#ifndef AFFICHAGE_RAYLIB_H
#define AFFICHAGE_RAYLIB_H

#include "simulation.h"

/* Dimensions de la fenêtre */
#define TAILLE_CASE      24
#define HAUT_BANDEAU     44                              /* bandeau du haut (titre, temps) */
#define LARGEUR_GRILLE   (NB_COLONNES * TAILLE_CASE)
#define HAUTEUR_GRILLE   (NB_LIGNES * TAILLE_CASE)
#define LARGEUR_PANNEAU  330
#define LARGEUR_FENETRE  (LARGEUR_GRILLE + LARGEUR_PANNEAU)
#define HAUTEUR_FENETRE  (HAUT_BANDEAU + HAUTEUR_GRILLE + 40)

void dessiner_menu(void);
void dessiner_simulation(const t_simulation *sim, int souris_x, int souris_y);
void dessiner_bilan(const t_simulation *sim);

/* Convertit la position de la souris en case de la grille. Renvoie 1 si elle est sur la grille. */
int souris_vers_case(int x, int y, t_position *p);

#endif
