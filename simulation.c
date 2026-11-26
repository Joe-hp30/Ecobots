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

/* Réserve de départ. Symboles :
 *   '.' libre   '+' station de recharge   'H' maison du technicien
 *   'P' 'V' 'M' centres de tri (plastique, verre, métal)
 *   '#' forêt dense   '^' falaise   '~' plan d'eau   (obstacles infranchissables)
 * Elle pourra être remplacée par une génération aléatoire (extension E1). */
static const char *RESERVE_INITIALE[NB_LIGNES] = {
    "........................................",
    ".P.....####..........^^^...........+....",
    "...+..#####.........^^^^^.......####....",
    "......####...........^^^........####....",
    "........................................",
    "...##..........~.........##.........^^..",
    "...##.........~~~........##.........^^..",
    "..............~~~~~...................V.",
    "...............~~~~.....................",
    ".......^^^..H......~~....#.....~~~......",
    "......^^^^^.........~~..###...~~~~~.....",
    ".......^^^...........~...#..M..~~~......",
    "....................................P...",
    "..##.....................^^.......##....",
    "..##.....................^^.......##....",
    "........+................+..............",
    "....###..........~~~..............^^^...",
    "...#####........~~~~~.............^^^.+.",
    "....###...........~~~........V..........",
    "..M....................................."
};

static const int DL[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

/* Écrit un message pour l'IHM sans dépasser le tableau */
static void ecrire_message(t_simulation *sim, const char *texte)
{
    strncpy(sim->message, texte, TAILLE_MESSAGE - 1);
    sim->message[TAILLE_MESSAGE - 1] = '\0';
}

static int meme_position(t_position a, t_position b)
{
    return a.lig == b.lig && a.col == b.col;
}

static const char *NOM_TYPE[NB_TYPES_DECHETS] = {"plastique", "verre", "metal"};

/* ---------- Initialisation ---------- */

static void init_engin(t_engin *engin, t_position depart)
{
    memset(engin, 0, sizeof(*engin));
    engin->pos = depart;
    engin->batterie = BATTERIE_MAX;
    engin->etat = OPERATIONNEL;
}

void init_simulation(t_simulation *sim)
{
    int l, c;
    t_position depart = {0, 0};
    int depart_trouve = 0;

    if (sim == NULL) {
        return;
    }
    memset(sim, 0, sizeof(*sim));   /* aucune valeur au hasard en mémoire */

    for (l = 0; l < NB_LIGNES; l++) {
        for (c = 0; c < NB_COLONNES; c++) {
            switch (RESERVE_INITIALE[l][c]) {
                case '#': sim->grille[l][c] = FORET; break;
                case '^': sim->grille[l][c] = FALAISE; break;
                case '~': sim->grille[l][c] = EAU; break;
                case 'P': sim->grille[l][c] = CENTRE_PLASTIQUE; break;
                case 'V': sim->grille[l][c] = CENTRE_VERRE; break;
                case 'M': sim->grille[l][c] = CENTRE_METAL; break;
                case '+': sim->grille[l][c] = STATION; break;
                case 'H':
                    sim->grille[l][c] = MAISON;
                    sim->technicien.maison.lig = l;
                    sim->technicien.maison.col = c;
                    break;
                default:  sim->grille[l][c] = LIBRE; break;
            }
            /* Les engins démarrent sur le premier centre de tri trouvé */
            if (!depart_trouve && est_centre(sim->grille[l][c])) {
                depart.lig = l;
                depart.col = c;
                depart_trouve = 1;
            }
        }
    }

    init_engin(&sim->drone, depart);
    init_engin(&sim->robot, depart);
    sim->technicien.pos = sim->technicien.maison;
    sim->technicien.etat = A_LA_MAISON;
    sim->engin_actif = DRONE;
    sim->en_cours = 1;
    ecrire_message(sim, "Cliquez sur une case pour y envoyer le drone.");
}

/* ---------- Tests sur les cases ---------- */

/* 1 si (lig, col) est dans la grille. À appeler AVANT tout accès à grille[lig][col]. */
int est_dans_grille(int lig, int col)
{
    return lig >= 0 && lig < NB_LIGNES && col >= 0 && col < NB_COLONNES;
}

/* 1 si le terrain est un obstacle naturel (forêt, falaise, plan d'eau) */
int est_obstacle(t_case terrain)
{
    return terrain == FORET || terrain == FALAISE || terrain == EAU;
}

int est_centre(t_case terrain)
{
    return terrain == CENTRE_PLASTIQUE || terrain == CENTRE_VERRE || terrain == CENTRE_METAL;
}

/* Type de déchets accepté par un centre de tri (à n'appeler que sur un centre) */
t_type_dechet type_du_centre(t_case terrain)
{
    if (terrain == CENTRE_VERRE) {
        return VERRE;
    }
    if (terrain == CENTRE_METAL) {
        return METAL;
    }
    return PLASTIQUE;
}

/* Un engin peut aller partout sauf sur les obstacles et dans la maison */
int case_franchissable(const t_simulation *sim, int lig, int col)
{
    if (sim == NULL || !est_dans_grille(lig, col)) {
        return 0;
    }
    return !est_obstacle(sim->grille[lig][col]) && sim->grille[lig][col] != MAISON;
}

/* Le technicien peut aller partout sauf sur les obstacles */
int case_praticable_a_pied(const t_simulation *sim, int lig, int col)
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

/* ---------- Plus courts chemins ---------- */

static int praticable(const t_simulation *sim, int lig, int col, int a_pied)
{
    return a_pied ? case_praticable_a_pied(sim, lig, col) : case_franchissable(sim, lig, col);
}

/* Remplit dist[l][c] = nombre de pas pour aller de (l, c) jusqu'à "cible",
 * ou -1 si c'est impossible. Parcours en largeur avec une file. */
static void calculer_distances(const t_simulation *sim, t_position cible, int a_pied,
                               int dist[NB_LIGNES][NB_COLONNES])
{
    t_position file[NB_LIGNES * NB_COLONNES];
    int debut = 0, fin = 0;
    int l, c, k;

    for (l = 0; l < NB_LIGNES; l++) {
        for (c = 0; c < NB_COLONNES; c++) {
            dist[l][c] = -1;
        }
    }
    if (!praticable(sim, cible.lig, cible.col, a_pied)) {
        return;
    }

    dist[cible.lig][cible.col] = 0;
    file[fin++] = cible;
    while (debut < fin) {
        t_position p = file[debut++];
        for (k = 0; k < 4; k++) {
            int nl = p.lig + DL[k];
            int nc = p.col + DC[k];
            if (praticable(sim, nl, nc, a_pied) && dist[nl][nc] == -1) {
                dist[nl][nc] = dist[p.lig][p.col] + 1;
                file[fin].lig = nl;
                file[fin].col = nc;
                fin++;
            }
        }
    }
}

int distance_trajet(const t_simulation *sim, t_position depart, t_position arrivee, int a_pied)
{
    int dist[NB_LIGNES][NB_COLONNES];

    if (sim == NULL || !est_dans_grille(depart.lig, depart.col)) {
        return -1;
    }
    calculer_distances(sim, arrivee, a_pied, dist);
    return dist[depart.lig][depart.col];
}

/* Case suivante sur le plus court chemin de "depart" vers "arrivee".
 * Renvoie "depart" si on est déjà arrivé ou si aucun chemin n'existe. */
t_position prochain_pas(const t_simulation *sim, t_position depart, t_position arrivee, int a_pied)
{
    int dist[NB_LIGNES][NB_COLONNES];
    int k;

    if (sim == NULL || !est_dans_grille(depart.lig, depart.col)) {
        return depart;
    }
    calculer_distances(sim, arrivee, a_pied, dist);
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

/* Station la plus proche de "depart" (à pied). Renvoie 0 s'il n'y en a aucune d'accessible. */
static int station_la_plus_proche(const t_simulation *sim, t_position depart, t_position *station)
{
    int dist[NB_LIGNES][NB_COLONNES];
    int l, c, meilleure = -1;

    calculer_distances(sim, depart, 1, dist);
    for (l = 0; l < NB_LIGNES; l++) {
        for (c = 0; c < NB_COLONNES; c++) {
            if (sim->grille[l][c] == STATION && dist[l][c] >= 0
                && (meilleure == -1 || dist[l][c] < meilleure)) {
                meilleure = dist[l][c];
                station->lig = l;
                station->col = c;
            }
        }
    }
    return meilleure != -1;
}

/* ---------- Engins ---------- */

t_engin *engin_pilote(t_simulation *sim)
{
    return (sim->engin_actif == DRONE) ? &sim->drone : &sim->robot;
}

t_engin *engin_de_type(t_simulation *sim, t_type_engin type)
{
    return (type == DRONE) ? &sim->drone : &sim->robot;
}

const char *nom_engin(const t_simulation *sim, const t_engin *engin)
{
    return (sim != NULL && engin == &sim->drone) ? "drone" : "robot";
}

int charge_totale(const t_engin *engin)
{
    int t, total = 0;

    for (t = 0; t < NB_TYPES_DECHETS; t++) {
        total += engin->charge[t];
    }
    return total;
}

/* Déplacement d'une case dans une direction. Renvoie 1 si l'engin a bougé, 0 sinon. */
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

/* Explique pourquoi un engin ne peut pas être piloté. Renvoie 1 s'il est pilotable. */
static int verifier_pilotable(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];

    switch (engin->etat) {
        case OPERATIONNEL:
            return 1;
        case EN_RECHARGE:
            snprintf(texte, sizeof(texte), "Le %s se recharge (encore %d s).", nom_engin(sim, engin),
                     (engin->temps_recharge + TICKS_PAR_SECONDE - 1) / TICKS_PAR_SECONDE);
            break;
        case EN_PANNE:
            snprintf(texte, sizeof(texte), "Le %s est en panne : le technicien arrive (Tab : autre engin).",
                     nom_engin(sim, engin));
            break;
        default:
            snprintf(texte, sizeof(texte), "Le %s est ramene a la station par le technicien.",
                     nom_engin(sim, engin));
            break;
    }
    ecrire_message(sim, texte);
    return 0;
}

/* Clic sur une case : l'engin piloté s'y rendra seul par le plus court chemin.
 * Renvoie 1 si le trajet est lancé. */
int definir_destination(t_simulation *sim, int lig, int col)
{
    t_engin *engin;
    t_position cible;

    if (sim == NULL || !est_dans_grille(lig, col)) {
        return 0;
    }
    engin = engin_pilote(sim);
    if (!verifier_pilotable(sim, engin)) {
        return 0;
    }

    cible.lig = lig;
    cible.col = col;
    if (distance_trajet(sim, engin->pos, cible, 0) < 0) {
        ecrire_message(sim, "Destination inaccessible (obstacle ou maison).");
        return 0;
    }

    engin->destination = cible;
    engin->a_destination = !meme_position(engin->pos, cible);
    engin->attente_pas = 0;
    ecrire_message(sim, engin->a_destination ? "En route !" : "Deja sur place.");
    return engin->a_destination;
}

/* ---------- Déchets ---------- */

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
                sim->stats.nb_detectes[sim->dechets[i].type]++;
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
    char texte[TAILLE_MESSAGE];
    int i;

    if (sim == NULL) {
        return 0;
    }

    i = indice_dechet_en(sim, sim->robot.pos.lig, sim->robot.pos.col);
    if (i == -1 || !sim->dechets[i].detecte) {
        return 0;   /* pas de déchet, ou déchet pas encore repéré par le drone */
    }
    if (charge_totale(&sim->robot) >= CAPACITE_ROBOT) {
        ecrire_message(sim, "Robot plein : videz-le dans les centres de tri.");
        return 0;
    }

    sim->dechets[i].actif = 0;
    sim->robot.charge[sim->dechets[i].type]++;
    snprintf(texte, sizeof(texte), "Dechet (%s) ramasse : %d/%d dans le robot.",
             NOM_TYPE[sim->dechets[i].type], charge_totale(&sim->robot), CAPACITE_ROBOT);
    ecrire_message(sim, texte);
    return 1;
}

/* Sur un centre de tri, le robot dépose les déchets du type accepté par ce centre.
 * Renvoie le nombre de déchets déposés. */
int deposer_dechets(t_simulation *sim)
{
    char texte[TAILLE_MESSAGE];
    t_case terrain;
    t_type_dechet type;
    int nb;

    if (sim == NULL || charge_totale(&sim->robot) == 0) {
        return 0;
    }
    terrain = sim->grille[sim->robot.pos.lig][sim->robot.pos.col];
    if (!est_centre(terrain)) {
        return 0;
    }

    type = type_du_centre(terrain);
    nb = sim->robot.charge[type];
    if (nb == 0) {
        snprintf(texte, sizeof(texte), "Ce centre ne trie que le %s : rien a deposer ici.", NOM_TYPE[type]);
        ecrire_message(sim, texte);
        return 0;
    }

    sim->robot.charge[type] = 0;
    sim->stats.nb_collectes[type] += nb;
    snprintf(texte, sizeof(texte), "%d dechet(s) de %s depose(s) au centre de tri.", nb, NOM_TYPE[type]);
    ecrire_message(sim, texte);
    return nb;
}

/* Ajoute un déchet non détecté en (lig, col) si la case le permet. Renvoie 1 si ajouté.
 * Utilisée par l'apparition aléatoire et par les tests. */
int ajouter_dechet(t_simulation *sim, int lig, int col, t_type_dechet type)
{
    int i, place = -1;

    if (sim == NULL || !est_dans_grille(lig, col) || type < 0 || type >= NB_TYPES_DECHETS) {
        return 0;
    }
    if (sim->grille[lig][col] != LIBRE || indice_dechet_en(sim, lig, col) != -1) {
        return 0;   /* seulement sur une case libre, sans autre déchet */
    }
    if (meme_position(sim->drone.pos, (t_position) {lig, col})
        || meme_position(sim->robot.pos, (t_position) {lig, col})) {
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
    sim->dechets[place].type = type;
    sim->dechets[place].detecte = 0;
    sim->dechets[place].actif = 1;
    sim->stats.nb_apparus++;
    return 1;
}

/* Avec une probabilité de PROBA_APPARITION pour mille, fait apparaître un déchet
 * de type aléatoire sur une case libre. Au plus MAX_ESSAIS_CASE tirages. */
int faire_apparaitre_dechet(t_simulation *sim)
{
    int essai;

    if (sim == NULL || rand() % 1000 >= PROBA_APPARITION) {
        return 0;
    }
    for (essai = 0; essai < MAX_ESSAIS_CASE; essai++) {
        if (ajouter_dechet(sim, rand() % NB_LIGNES, rand() % NB_COLONNES,
                           (t_type_dechet) (rand() % NB_TYPES_DECHETS))) {
            return 1;
        }
    }
    return 0;
}

/* ---------- Batteries, recharges et pannes ---------- */

/* % de batterie consommés par un pas : le drone vole et consomme plus ;
 * le robot consomme plus quand il est chargé. */
int consommation_deplacement(const t_simulation *sim, const t_engin *engin)
{
    if (sim == NULL || engin == NULL) {
        return 0;
    }
    if (engin == &sim->drone) {
        return CONSO_DRONE;
    }
    return CONSO_ROBOT_VIDE + CONSO_ROBOT_PAR_DECHET * charge_totale(engin);
}

/* L'engin s'immobilise DUREE_RECHARGE ticks sur la station ; sa batterie remonte peu à peu. */
void demarrer_recharge(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];

    if (sim == NULL || engin == NULL) {
        return;
    }
    engin->etat = EN_RECHARGE;
    engin->a_destination = 0;
    engin->temps_recharge = DUREE_RECHARGE;
    engin->batterie_debut_recharge = engin->batterie;
    sim->stats.nb_recharges++;
    snprintf(texte, sizeof(texte), "Recharge du %s : %d secondes.", nom_engin(sim, engin),
             DUREE_RECHARGE / TICKS_PAR_SECONDE);
    ecrire_message(sim, texte);
}

/* Batterie vide : l'engin s'arrête sur place et attend le technicien. */
void declencher_panne(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];

    if (sim == NULL || engin == NULL) {
        return;
    }
    engin->batterie = 0;
    engin->etat = EN_PANNE;
    engin->a_destination = 0;
    sim->stats.nb_pannes++;
    snprintf(texte, sizeof(texte), "Panne de batterie du %s ! Le technicien quitte sa maison.",
             nom_engin(sim, engin));
    ecrire_message(sim, texte);
}

/* Ce qui se passe quand un engin arrive sur une case (pas manuel ou trajet automatique) */
static void arriver_sur_case(t_simulation *sim, t_engin *engin, int conso)
{
    char texte[TAILLE_MESSAGE];

    engin->batterie -= conso;
    if (engin->batterie < 0) {
        engin->batterie = 0;
    }

    if (engin == &sim->drone) {
        detecter_dechets(sim);
    } else if (!deposer_dechets(sim)) {
        ramasser_dechet(sim);
    }

    if (sim->grille[engin->pos.lig][engin->pos.col] == STATION && engin->batterie < BATTERIE_MAX) {
        demarrer_recharge(sim, engin);
    } else if (engin->batterie == 0) {
        declencher_panne(sim, engin);
    } else if (engin->batterie <= SEUIL_BATTERIE_FAIBLE && engin->batterie + conso > SEUIL_BATTERIE_FAIBLE) {
        snprintf(texte, sizeof(texte), "Batterie faible du %s (%d %%) : allez sur une station (+).",
                 nom_engin(sim, engin), engin->batterie);
        ecrire_message(sim, texte);
    }
}

/* ---------- Commandes de l'utilisateur ---------- */

void appliquer_commande(t_simulation *sim, t_commande cmd)
{
    t_engin *engin;
    int conso;

    if (sim == NULL || !sim->en_cours) {
        return;
    }

    switch (cmd) {
        case QUITTER:
            sim->en_cours = 0;
            break;

        case CHANGER_ENGIN:
            sim->engin_actif = (sim->engin_actif == DRONE) ? ROBOT : DRONE;
            ecrire_message(sim, sim->engin_actif == DRONE ? "Vous pilotez le DRONE."
                                                          : "Vous pilotez le ROBOT.");
            break;

        case HAUT:
        case BAS:
        case GAUCHE:
        case DROITE:
            engin = engin_pilote(sim);
            if (!verifier_pilotable(sim, engin)) {
                break;
            }
            engin->a_destination = 0;   /* une touche annule le trajet automatique */
            conso = consommation_deplacement(sim, engin);   /* avant un éventuel ramassage */
            if (!deplacer_engin(sim, engin, cmd)) {
                ecrire_message(sim, "Deplacement impossible (obstacle, maison ou bord).");
                break;
            }
            sim->message[0] = '\0';
            arriver_sur_case(sim, engin, conso);
            break;

        default:
            break;   /* AUCUNE, CLIC (géré par definir_destination) : rien à faire ici */
    }
}

/* ---------- Le temps qui passe ---------- */

/* Trajet automatique (clic) et recharge d'un engin, pendant un tick */
static void mettre_a_jour_engin(t_simulation *sim, t_engin *engin)
{
    char texte[TAILLE_MESSAGE];
    t_position suivant;
    int conso;

    if (engin->etat == EN_RECHARGE) {
        engin->temps_recharge--;
        engin->batterie = engin->batterie_debut_recharge
                        + (BATTERIE_MAX - engin->batterie_debut_recharge)
                          * (DUREE_RECHARGE - engin->temps_recharge) / DUREE_RECHARGE;
        if (engin->temps_recharge <= 0) {
            engin->batterie = BATTERIE_MAX;
            engin->etat = OPERATIONNEL;
            snprintf(texte, sizeof(texte), "Le %s est recharge a 100 %%.", nom_engin(sim, engin));
            ecrire_message(sim, texte);
        }
        return;
    }

    if (engin->etat != OPERATIONNEL || !engin->a_destination) {
        return;
    }
    if (engin->attente_pas > 0) {
        engin->attente_pas--;
        return;
    }
    engin->attente_pas = TICKS_PAR_CASE_ENGIN - 1;

    suivant = prochain_pas(sim, engin->pos, engin->destination, 0);
    if (meme_position(suivant, engin->pos)) {
        engin->a_destination = 0;   /* arrivé, ou plus de chemin */
        return;
    }
    conso = consommation_deplacement(sim, engin);
    engin->pos = suivant;
    arriver_sur_case(sim, engin, conso);
    if (meme_position(engin->pos, engin->destination)) {
        engin->a_destination = 0;
    }
}

/* Renvoie 1 et remplit "cible" si un engin attend le technicien */
static int engin_en_attente(t_simulation *sim, t_type_engin *cible)
{
    if (sim->drone.etat == EN_PANNE) {
        *cible = DRONE;
        return 1;
    }
    if (sim->robot.etat == EN_PANNE) {
        *cible = ROBOT;
        return 1;
    }
    return 0;
}

/* Le technicien, pendant un tick : il part de sa maison, va chercher l'engin en panne,
 * le porte jusqu'à la station la plus proche, puis rentre chez lui. */
static void mettre_a_jour_technicien(t_simulation *sim)
{
    t_technicien *tech = &sim->technicien;
    t_engin *engin;
    char texte[TAILLE_MESSAGE];
    t_type_engin cible;

    /* Libre (chez lui ou sur le retour) : une panne le fait partir */
    if ((tech->etat == A_LA_MAISON || tech->etat == RENTRE) && engin_en_attente(sim, &cible)) {
        tech->etat = VA_CHERCHER;
        tech->cible = cible;
        tech->attente_pas = 0;
    }
    if (tech->etat == A_LA_MAISON) {
        return;
    }

    if (tech->attente_pas > 0) {
        tech->attente_pas--;
        return;
    }
    tech->attente_pas = TICKS_PAR_CASE_PIETON - 1;
    engin = engin_de_type(sim, tech->cible);

    switch (tech->etat) {
        case VA_CHERCHER:
            tech->pos = prochain_pas(sim, tech->pos, engin->pos, 1);
            if (meme_position(tech->pos, engin->pos)) {
                if (!station_la_plus_proche(sim, engin->pos, &tech->station)) {
                    tech->station = engin->pos;   /* aucune station : on recharge sur place */
                }
                engin->etat = TRANSPORTE;
                tech->etat = RAMENE;
                snprintf(texte, sizeof(texte), "Le technicien a recupere le %s.", nom_engin(sim, engin));
                ecrire_message(sim, texte);
            }
            break;

        case RAMENE:
            tech->pos = prochain_pas(sim, tech->pos, tech->station, 1);
            engin->pos = tech->pos;   /* il porte l'engin */
            if (meme_position(tech->pos, tech->station)) {
                demarrer_recharge(sim, engin);
                tech->etat = RENTRE;
                snprintf(texte, sizeof(texte), "Le %s est depose a la station ; le technicien rentre.",
                         nom_engin(sim, engin));
                ecrire_message(sim, texte);
            }
            break;

        case RENTRE:
            tech->pos = prochain_pas(sim, tech->pos, tech->maison, 1);
            if (meme_position(tech->pos, tech->maison)) {
                tech->etat = A_LA_MAISON;
            }
            break;

        default:
            break;
    }
}

void avancer_temps(t_simulation *sim)
{
    if (sim == NULL || !sim->en_cours) {
        return;
    }
    mettre_a_jour_engin(sim, &sim->drone);
    mettre_a_jour_engin(sim, &sim->robot);
    mettre_a_jour_technicien(sim);
    faire_apparaitre_dechet(sim);
    sim->stats.nb_ticks++;
}

/* ---------- Informations pour l'affichage ---------- */

int nb_dechets_en_attente_type(const t_simulation *sim, t_type_dechet type)
{
    int i, n = 0;

    if (sim == NULL) {
        return 0;
    }
    for (i = 0; i < sim->nb_dechets; i++) {
        if (sim->dechets[i].actif && sim->dechets[i].detecte && sim->dechets[i].type == type) {
            n++;
        }
    }
    return n;
}

int nb_dechets_en_attente(const t_simulation *sim)
{
    int t, n = 0;

    for (t = 0; t < NB_TYPES_DECHETS; t++) {
        n += nb_dechets_en_attente_type(sim, (t_type_dechet) t);
    }
    return n;
}
