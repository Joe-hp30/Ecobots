/*
 * simulation.c — Moteur de la simulation EcoBots
 *
 * Aucune fonction de ce fichier n'affiche ou ne lit quoi que ce soit.
 * Les messages destinés à l'utilisateur sont écrits dans sim->message ;
 * c'est le module affichage qui décide comment les montrer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulation.h"

/* Réserve de départ (jeu de base). Symboles :
 *   '.' libre   'C' centre de collecte
 *   '#' forêt dense   '^' falaise   '~' plan d'eau   (obstacles infranchissables)
 * Elle sera remplacée par une génération aléatoire dans l'extension E1. */
static const char *RESERVE_INITIALE[NB_LIGNES] = {
    "..............................",
    ".C.....####..........^^^......",
    "......#####.........^^^^^.....",
    "......####...........^^^......",
    "..............................",
    "...##..........~.........##...",
    "...##.........~~~........##...",
    "..............~~~~~...........",
    "...............~~~~...........",
    ".......^^^.........~~....#....",
    "......^^^^^.........~~..###...",
    ".......^^^...........~...#..C.",
    "..............................",
    "..##.....................^^...",
    "..##.....................^^..."
};

/* Petite fonction interne : écrit un message pour l'IHM sans dépasser le tableau */
static void ecrire_message(t_simulation *sim, const char *texte)
{
    strncpy(sim->message, texte, TAILLE_MESSAGE - 1);
    sim->message[TAILLE_MESSAGE - 1] = '\0';
}

/* ---------- Initialisation ---------- */

void init_simulation(t_simulation *sim)
{
    int l, c;
    int centre_trouve = 0;

    if (sim == NULL) {
        return;
    }

    /* On met toute la structure à zéro : aucune valeur au hasard en mémoire */
    memset(sim, 0, sizeof(*sim));

    for (l = 0; l < NB_LIGNES; l++) {
        for (c = 0; c < NB_COLONNES; c++) {
            char symbole = RESERVE_INITIALE[l][c];
            if (symbole == '#') {
                sim->grille[l][c] = FORET;
            } else if (symbole == '^') {
                sim->grille[l][c] = FALAISE;
            } else if (symbole == '~') {
                sim->grille[l][c] = EAU;
            } else if (symbole == 'C') {
                sim->grille[l][c] = CENTRE;
                /* Le drone et le robot démarrent sur le premier centre trouvé */
                if (!centre_trouve) {
                    sim->drone.pos.lig = l;
                    sim->drone.pos.col = c;
                    sim->robot.pos = sim->drone.pos;
                    centre_trouve = 1;
                }
            } else {
                sim->grille[l][c] = LIBRE;
            }
        }
    }

    sim->engin_actif = DRONE;
    sim->en_cours = 1;
    ecrire_message(sim, "Simulation lancee : explorez la reserve avec le drone.");
}

/* ---------- Tests sur les cases ---------- */

/* 1 si (lig, col) est dans la grille, 0 sinon. À appeler AVANT tout accès à grille[lig][col]. */
int est_dans_grille(int lig, int col)
{
    return lig >= 0 && lig < NB_LIGNES && col >= 0 && col < NB_COLONNES;
}

/* 1 si le terrain est un obstacle naturel (forêt, falaise, plan d'eau) */
int est_obstacle(t_case terrain)
{
    return terrain == FORET || terrain == FALAISE || terrain == EAU;
}

/* 1 si un engin peut aller sur la case (dans la grille et pas un obstacle) */
int case_franchissable(const t_simulation *sim, int lig, int col)
{
    if (sim == NULL || !est_dans_grille(lig, col)) {
        return 0;
    }
    return !est_obstacle(sim->grille[lig][col]);
}

/* Indice du déchet actif situé en (lig, col), ou -1 s'il n'y en a pas */
int indice_dechet_en(const t_simulation *sim, int lig, int col)
{
    int i;

    if (sim == NULL) {
        return -1;
    }
    for (i = 0; i < sim->nb_dechets; i++) {
        if (sim->dechets[i].actif
            && sim->dechets[i].pos.lig == lig
            && sim->dechets[i].pos.col == col) {
            return i;
        }
    }
    return -1;
}

/* 1 s'il y a en (lig, col) un déchet actif ET détecté (donc visible à l'écran) */
int dechet_visible_en(const t_simulation *sim, int lig, int col)
{
    int i = indice_dechet_en(sim, lig, col);
    return i != -1 && sim->dechets[i].detecte;
}

/* ---------- Déplacement ---------- */

int deplacer_engin(t_simulation *sim, t_engin *engin, t_commande direction)
{
    int lig, col;

    if (sim == NULL || engin == NULL) {
        return 0;
    }

    lig = engin->pos.lig;
    col = engin->pos.col;

    switch (direction) {
        case HAUT:   lig--; break;
        case BAS:    lig++; break;
        case GAUCHE: col--; break;
        case DROITE: col++; break;
        default:     return 0;   /* pas une direction : on ne bouge pas */
    }

    if (!case_franchissable(sim, lig, col)) {
        return 0;
    }

    engin->pos.lig = lig;
    engin->pos.col = col;
    return 1;
}

/* ---------- Règles ---------- */

/* Scanne la case du drone et ses 8 voisines. Renvoie le nombre de NOUVEAUX déchets détectés. */
int detecter_dechets(t_simulation *sim)
{
    int dl, dc, i;
    int nouveaux = 0;

    if (sim == NULL) {
        return 0;
    }

    for (dl = -1; dl <= 1; dl++) {
        for (dc = -1; dc <= 1; dc++) {
            int lig = sim->drone.pos.lig + dl;
            int col = sim->drone.pos.col + dc;

            if (!est_dans_grille(lig, col)) {
                continue;   /* le drone longe un bord : cette voisine n'existe pas */
            }
            i = indice_dechet_en(sim, lig, col);
            if (i != -1 && !sim->dechets[i].detecte) {
                sim->dechets[i].detecte = 1;
                sim->stats.nb_detectes++;
                nouveaux++;
            }
        }
    }

    if (nouveaux > 0) {
        char texte[TAILLE_MESSAGE];
        snprintf(texte, sizeof(texte), "%d dechet(s) detecte(s) autour de (%d, %d) !",
                 nouveaux, sim->drone.pos.lig, sim->drone.pos.col);
        ecrire_message(sim, texte);
    }
    return nouveaux;
}

/* Le robot vide ramasse le déchet détecté sur sa case. Renvoie 1 si ramassage. */
int ramasser_dechet(t_simulation *sim)
{
    int i;

    if (sim == NULL) {
        return 0;
    }

    i = indice_dechet_en(sim, sim->robot.pos.lig, sim->robot.pos.col);
    if (i == -1 || !sim->dechets[i].detecte) {
        return 0;   /* pas de déchet, ou déchet pas encore repéré par le drone */
    }
    if (sim->robot.charge) {
        ecrire_message(sim, "Robot deja plein : deposez d'abord le dechet au centre.");
        return 0;
    }

    sim->dechets[i].actif = 0;
    sim->robot.charge = 1;
    ecrire_message(sim, "Dechet ramasse. Direction un centre de collecte (C).");
    return 1;
}

/* Le robot chargé dépose son déchet s'il est sur un centre. Renvoie 1 si dépôt. */
int deposer_dechet(t_simulation *sim)
{
    if (sim == NULL || !sim->robot.charge) {
        return 0;
    }
    if (sim->grille[sim->robot.pos.lig][sim->robot.pos.col] != CENTRE) {
        return 0;
    }

    sim->robot.charge = 0;
    sim->stats.nb_collectes++;
    ecrire_message(sim, "Dechet depose au centre de collecte.");
    return 1;
}

/* Ajoute un déchet non détecté en (lig, col) si la case le permet. Renvoie 1 si ajouté.
 * Utilisée par l'apparition aléatoire et par les tests (pour placer un déchet à la main). */
int ajouter_dechet(t_simulation *sim, int lig, int col)
{
    int i, place = -1;

    if (sim == NULL || !est_dans_grille(lig, col)) {
        return 0;
    }
    if (sim->grille[lig][col] != LIBRE || indice_dechet_en(sim, lig, col) != -1) {
        return 0;   /* pas sur un obstacle, un centre ou un autre déchet */
    }
    if ((sim->drone.pos.lig == lig && sim->drone.pos.col == col)
        || (sim->robot.pos.lig == lig && sim->robot.pos.col == col)) {
        return 0;   /* pas sous un engin */
    }

    /* On réutilise d'abord la place d'un déchet déjà ramassé... */
    for (i = 0; i < sim->nb_dechets && place == -1; i++) {
        if (!sim->dechets[i].actif) {
            place = i;
        }
    }
    /* ...sinon on prend une nouvelle case du tableau, s'il en reste */
    if (place == -1) {
        if (sim->nb_dechets >= MAX_DECHETS) {
            return 0;   /* tableau plein : on n'écrit jamais au-delà */
        }
        place = sim->nb_dechets;
        sim->nb_dechets++;
    }

    sim->dechets[place].pos.lig = lig;
    sim->dechets[place].pos.col = col;
    sim->dechets[place].detecte = 0;
    sim->dechets[place].actif = 1;
    sim->stats.nb_apparus++;
    return 1;
}

/* Avec une probabilité de PROBA_APPARITION %, fait apparaître un déchet sur une case libre.
 * Au plus MAX_ESSAIS_CASE tirages : pas de boucle infinie si la grille est pleine. */
int faire_apparaitre_dechet(t_simulation *sim)
{
    int essai;

    if (sim == NULL || rand() % 100 >= PROBA_APPARITION) {
        return 0;
    }
    for (essai = 0; essai < MAX_ESSAIS_CASE; essai++) {
        int lig = rand() % NB_LIGNES;
        int col = rand() % NB_COLONNES;
        if (ajouter_dechet(sim, lig, col)) {
            return 1;
        }
    }
    return 0;
}

/* ---------- Un tour de simulation ---------- */

void appliquer_commande(t_simulation *sim, t_commande cmd)
{
    if (sim == NULL || !sim->en_cours) {
        return;
    }

    switch (cmd) {
        case QUITTER:
            sim->en_cours = 0;
            return;

        case CHANGER_ENGIN:
            sim->engin_actif = (sim->engin_actif == DRONE) ? ROBOT : DRONE;
            ecrire_message(sim, sim->engin_actif == DRONE ? "Vous pilotez le DRONE."
                                                          : "Vous pilotez le ROBOT.");
            break;

        case HAUT:
        case BAS:
        case GAUCHE:
        case DROITE:
            sim->message[0] = '\0';
            if (!deplacer_engin(sim, sim->engin_actif == DRONE ? &sim->drone : &sim->robot, cmd)) {
                ecrire_message(sim, "Deplacement impossible (obstacle ou bord).");
                return;   /* l'engin n'a pas bougé : pas de tour joué */
            }
            if (sim->engin_actif == DRONE) {
                detecter_dechets(sim);
            } else if (!deposer_dechet(sim)) {
                ramasser_dechet(sim);
            }
            break;

        default:
            return;   /* AUCUNE ou valeur inconnue : rien ne se passe, pas de tour joué */
    }

    /* La réserve évolue à chaque tour joué */
    faire_apparaitre_dechet(sim);
    sim->stats.nb_tours++;
}

int nb_dechets_en_attente(const t_simulation *sim)
{
    int i, n = 0;

    if (sim == NULL) {
        return 0;
    }
    for (i = 0; i < sim->nb_dechets; i++) {
        if (sim->dechets[i].actif && sim->dechets[i].detecte) {
            n++;
        }
    }
    return n;
}
