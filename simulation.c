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
 *   '.' libre   'C' centre de collecte   '+' station de recharge
 *   '#' forêt dense   '^' falaise   '~' plan d'eau   (obstacles infranchissables)
 * Elle sera remplacée par une génération aléatoire dans l'extension E1. */
static const char *RESERVE_INITIALE[NB_LIGNES] = {
    "........................................",
    ".C.....####..........^^^...........+....",
    "...+..#####.........^^^^^.......####....",
    "......####...........^^^........####....",
    "........................................",
    "...##..........~.........##.........^^..",
    "...##.........~~~........##.........^^..",
    "..............~~~~~...................C.",
    "...............~~~~.....................",
    ".......^^^.........~~....#.....~~~......",
    "......^^^^^.........~~..###...~~~~~.....",
    ".......^^^...........~...#..C..~~~......",
    "........................................",
    "..##.....................^^.......##....",
    "..##.....................^^.......##....",
    "........+................+..............",
    "....###..........~~~..............^^^...",
    "...#####........~~~~~.............^^^.+.",
    "....###...........~~~........C..........",
    "........................................"
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
            } else if (symbole == '+') {
                sim->grille[l][c] = STATION;
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

    sim->drone.batterie = BATTERIE_MAX;
    sim->robot.batterie = BATTERIE_MAX;
    sim->drone.etat = OPERATIONNEL;
    sim->robot.etat = OPERATIONNEL;
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

/* ---------- Chemins à pied (techniciens) ---------- */

/* Remplit dist[l][c] = nombre de pas pour aller de (l, c) jusqu'à "cible" en contournant
 * les obstacles, ou -1 si c'est impossible. Parcours en largeur avec une file. */
static void calculer_distances(const t_simulation *sim, t_position cible,
                               int dist[NB_LIGNES][NB_COLONNES])
{
    static const int DL[4] = {-1, 1, 0, 0};
    static const int DC[4] = {0, 0, -1, 1};
    t_position file[NB_LIGNES * NB_COLONNES];
    int debut = 0, fin = 0;
    int l, c, k;

    for (l = 0; l < NB_LIGNES; l++) {
        for (c = 0; c < NB_COLONNES; c++) {
            dist[l][c] = -1;
        }
    }
    if (!case_franchissable(sim, cible.lig, cible.col)) {
        return;
    }

    dist[cible.lig][cible.col] = 0;
    file[fin++] = cible;
    while (debut < fin) {
        t_position p = file[debut++];
        for (k = 0; k < 4; k++) {
            int nl = p.lig + DL[k];
            int nc = p.col + DC[k];
            if (case_franchissable(sim, nl, nc) && dist[nl][nc] == -1) {
                dist[nl][nc] = dist[p.lig][p.col] + 1;
                file[fin].lig = nl;
                file[fin].col = nc;
                fin++;
            }
        }
    }
}

/* Nombre de pas à pied entre deux cases (-1 si inaccessible) */
int distance_trajet(const t_simulation *sim, t_position depart, t_position arrivee)
{
    int dist[NB_LIGNES][NB_COLONNES];

    if (sim == NULL || !est_dans_grille(depart.lig, depart.col)) {
        return -1;
    }
    calculer_distances(sim, arrivee, dist);
    return dist[depart.lig][depart.col];
}

/* Case suivante sur le plus court chemin de "depart" vers "arrivee".
 * Renvoie "depart" si on est déjà arrivé ou si aucun chemin n'existe. */
t_position prochain_pas(const t_simulation *sim, t_position depart, t_position arrivee)
{
    static const int DL[4] = {-1, 1, 0, 0};
    static const int DC[4] = {0, 0, -1, 1};
    int dist[NB_LIGNES][NB_COLONNES];
    int k;

    if (sim == NULL || !est_dans_grille(depart.lig, depart.col)) {
        return depart;
    }
    calculer_distances(sim, arrivee, dist);
    if (dist[depart.lig][depart.col] <= 0) {
        return depart;
    }
    for (k = 0; k < 4; k++) {
        int nl = depart.lig + DL[k];
        int nc = depart.col + DC[k];
        if (est_dans_grille(nl, nc) && dist[nl][nc] == dist[depart.lig][depart.col] - 1) {
            t_position suivant;
            suivant.lig = nl;
            suivant.col = nc;
            return suivant;
        }
    }
    return depart;
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

/* Le robot ramasse le déchet détecté sur sa case, s'il lui reste de la place. Renvoie 1 si ramassage. */
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
    if (sim->robot.charge >= CAPACITE_ROBOT) {
        ecrire_message(sim, "Robot plein : videz-le dans un centre de collecte (C).");
        return 0;
    }

    sim->dechets[i].actif = 0;
    sim->robot.charge++;
    {
        char texte[TAILLE_MESSAGE];
        snprintf(texte, sizeof(texte), "Dechet ramasse (%d/%d dans le robot).",
                 sim->robot.charge, CAPACITE_ROBOT);
        ecrire_message(sim, texte);
    }
    return 1;
}

/* Le robot chargé vide tous ses déchets s'il est sur un centre. Renvoie le nombre déposé. */
int deposer_dechet(t_simulation *sim)
{
    char texte[TAILLE_MESSAGE];
    int nb;

    if (sim == NULL || sim->robot.charge == 0) {
        return 0;
    }
    if (sim->grille[sim->robot.pos.lig][sim->robot.pos.col] != CENTRE) {
        return 0;
    }

    nb = sim->robot.charge;
    sim->robot.charge = 0;
    sim->stats.nb_collectes += nb;
    snprintf(texte, sizeof(texte), "%d dechet(s) depose(s) au centre de collecte.", nb);
    ecrire_message(sim, texte);
    return nb;
}

/* ---------- Batteries et pannes ---------- */

/* Nom de l'engin pour les messages ("drone" ou "robot") */
const char *nom_engin(const t_simulation *sim, const t_engin *engin)
{
    return (sim != NULL && engin == &sim->drone) ? "drone" : "robot";
}

/* % de batterie consommés par un déplacement : le drone vole et consomme plus ;
 * le robot consomme plus quand il est chargé. */
int consommation_deplacement(const t_simulation *sim, const t_engin *engin)
{
    if (sim == NULL || engin == NULL) {
        return 0;
    }
    if (engin == &sim->drone) {
        return CONSO_DRONE;
    }
    return CONSO_ROBOT_VIDE + CONSO_ROBOT_PAR_DECHET * engin->charge;
}

/* Recharge progressive : + RECHARGE_PAR_TOUR % par tour passé sur une station.
 * Renvoie 1 si l'engin est sur une station. */
int recharger_si_station(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];

    if (sim == NULL || engin == NULL || engin->etat != OPERATIONNEL) {
        return 0;
    }
    if (sim->grille[engin->pos.lig][engin->pos.col] != STATION) {
        return 0;
    }
    if (engin->batterie < BATTERIE_MAX) {
        engin->batterie += RECHARGE_PAR_TOUR;
        if (engin->batterie > BATTERIE_MAX) {
            engin->batterie = BATTERIE_MAX;
        }
        /* On ne prévient que pour l'engin piloté, pour ne pas cacher les autres messages */
        if (engin == (sim->engin_actif == DRONE ? &sim->drone : &sim->robot)) {
            snprintf(texte, sizeof(texte), "Recharge du %s : %d %%%s", nom_engin(sim, engin), engin->batterie,
                     engin->batterie < BATTERIE_MAX ? " (Espace pour attendre)" : ", batterie pleine.");
            ecrire_message(sim, texte);
        }
    }
    return 1;
}

/* Batterie vide : l'engin s'arrête sur place. Un technicien part à pied de la station
 * la plus proche (en suivant les chemins), vient le chercher et le ramène à cette station. */
void declencher_panne(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];
    int dist[NB_LIGNES][NB_COLONNES];
    int l, c, meilleure = -1;

    if (sim == NULL || engin == NULL) {
        return;
    }

    engin->batterie = 0;
    engin->etat = EN_PANNE;
    engin->station = engin->pos;
    sim->stats.nb_pannes++;

    calculer_distances(sim, engin->pos, dist);
    for (l = 0; l < NB_LIGNES; l++) {
        for (c = 0; c < NB_COLONNES; c++) {
            if (sim->grille[l][c] == STATION && dist[l][c] >= 0
                && (meilleure == -1 || dist[l][c] < meilleure)) {
                meilleure = dist[l][c];
                engin->station.lig = l;
                engin->station.col = c;
            }
        }
    }
    engin->technicien = engin->station;

    snprintf(texte, sizeof(texte), "Panne de batterie du %s ! Un technicien arrive a pied (%d cases).",
             nom_engin(sim, engin), meilleure < 0 ? 0 : meilleure);
    ecrire_message(sim, texte);
}

/* Fait avancer d'une case le technicien d'un engin en panne (appelée à chaque tour). */
void avancer_secours(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];

    if (sim == NULL || engin == NULL || engin->etat == OPERATIONNEL) {
        return;
    }
    sim->stats.nb_tours_immobilises++;

    if (engin->etat == EN_PANNE) {
        /* 1re étape : le technicien marche jusqu'à l'engin */
        engin->technicien = prochain_pas(sim, engin->technicien, engin->pos);
        if (engin->technicien.lig == engin->pos.lig && engin->technicien.col == engin->pos.col) {
            engin->etat = REMORQUE;
            snprintf(texte, sizeof(texte), "Le technicien a recupere le %s et le ramene a la station.",
                     nom_engin(sim, engin));
            ecrire_message(sim, texte);
        }
        return;
    }

    /* 2e étape (REMORQUE) : le technicien ramène l'engin à la station, case par case */
    engin->pos = prochain_pas(sim, engin->pos, engin->station);
    engin->technicien = engin->pos;
    if (engin->pos.lig == engin->station.lig && engin->pos.col == engin->station.col) {
        engin->etat = OPERATIONNEL;
        engin->batterie = BATTERIE_MAX;
        snprintf(texte, sizeof(texte), "Le %s est de retour a la station, batterie rechargee.",
                 nom_engin(sim, engin));
        ecrire_message(sim, texte);
    }
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

/* Déplace l'engin piloté. Renvoie 1 si un tour a été joué, 0 si le mouvement est refusé. */
static int jouer_deplacement(t_simulation *sim, t_commande direction)
{
    t_engin *engin = (sim->engin_actif == DRONE) ? &sim->drone : &sim->robot;
    char texte[TAILLE_MESSAGE];
    int conso;

    if (engin->etat != OPERATIONNEL) {
        snprintf(texte, sizeof(texte), "Le %s est en panne : Tab pour changer d'engin, Espace pour attendre.",
                 nom_engin(sim, engin));
        ecrire_message(sim, texte);
        return 0;
    }

    conso = consommation_deplacement(sim, engin);   /* calculée avant un éventuel ramassage */
    if (!deplacer_engin(sim, engin, direction)) {
        ecrire_message(sim, "Deplacement impossible (obstacle ou bord).");
        return 0;
    }

    sim->message[0] = '\0';
    engin->batterie -= conso;
    if (engin->batterie < 0) {
        engin->batterie = 0;
    }

    if (sim->engin_actif == DRONE) {
        detecter_dechets(sim);
    } else if (!deposer_dechet(sim)) {
        ramasser_dechet(sim);
    }

    if (engin->batterie == 0 && sim->grille[engin->pos.lig][engin->pos.col] != STATION) {
        declencher_panne(sim, engin);
    } else if (engin->batterie <= SEUIL_BATTERIE_FAIBLE && sim->message[0] == '\0') {
        snprintf(texte, sizeof(texte), "Batterie faible (%d %%) : rejoignez une station de recharge (+).",
                 engin->batterie);
        ecrire_message(sim, texte);
    }
    return 1;
}

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

        case ATTENDRE:
            ecrire_message(sim, "Un tour passe...");
            break;

        case HAUT:
        case BAS:
        case GAUCHE:
        case DROITE:
            if (!jouer_deplacement(sim, cmd)) {
                return;   /* mouvement refusé : pas de tour joué */
            }
            break;

        default:
            return;   /* AUCUNE ou valeur inconnue : rien ne se passe, pas de tour joué */
    }

    /* Fin du tour : recharges, techniciens, nouveaux déchets */
    recharger_si_station(sim, &sim->drone);
    recharger_si_station(sim, &sim->robot);
    avancer_secours(sim, &sim->drone);
    avancer_secours(sim, &sim->robot);
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
