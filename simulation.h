/*
 * simulation.h — Moteur de la simulation EcoBots
 *
 * Ce module contient TOUTES les données et TOUTES les règles de la simulation.
 * Il ne fait AUCUN affichage (pas de printf) et AUCUNE saisie clavier (pas de getch) :
 * on peut changer l'IHM (console, raylib...) sans modifier ce module.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

/* ---------- Constantes ---------- */

#define NB_LIGNES          20   /* hauteur de la grille */
#define NB_COLONNES        40   /* largeur de la grille */
#define MAX_DECHETS        80   /* nombre max de déchets en même temps */
#define PROBA_APPARITION   10   /* chance (en %) qu'un déchet apparaisse à chaque tour */
#define MAX_ESSAIS_CASE   100   /* essais max pour trouver une case libre au hasard */
#define TAILLE_MESSAGE     90   /* taille du message affiché par l'IHM */
#define CAPACITE_ROBOT      3   /* nombre de déchets que le robot transporte à la fois */
#define BATTERIE_MAX_DRONE 60   /* autonomie du drone, en déplacements */
#define BATTERIE_MAX_ROBOT 80   /* autonomie du robot, en déplacements */

/* ---------- Types ---------- */

/* Nature du terrain d'une case.
 * FORET, FALAISE et EAU sont des obstacles naturels infranchissables. */
typedef enum {
    LIBRE,
    FORET,
    FALAISE,
    EAU,
    CENTRE,
    STATION    /* station de recharge des batteries */
} t_case;

/* Commandes possibles, indépendantes des touches du clavier */
typedef enum {
    AUCUNE,
    HAUT,
    BAS,
    GAUCHE,
    DROITE,
    CHANGER_ENGIN,
    QUITTER
} t_commande;

/* Engin actuellement piloté */
typedef enum {
    DRONE,
    ROBOT
} t_type_engin;

/* Coordonnées d'une case dans la grille */
typedef struct {
    int lig;
    int col;
} t_position;

/* Un déchet de la réserve */
typedef struct {
    t_position pos;
    int detecte;   /* 1 si le drone l'a repéré, 0 sinon */
    int actif;     /* 1 s'il est encore au sol, 0 s'il a été ramassé */
} t_dechet;

/* Le drone ou le robot */
typedef struct {
    t_position pos;
    int charge;          /* robot seulement : nombre de déchets transportés (0 à CAPACITE_ROBOT) */
    int batterie;        /* déplacements restants avant la panne */
    int batterie_max;    /* autonomie quand la batterie est pleine */
} t_engin;

/* Compteurs pour le panneau d'infos et le bilan */
typedef struct {
    int nb_apparus;
    int nb_detectes;
    int nb_collectes;
    int nb_tours;
    int nb_pannes;       /* batteries tombées à zéro */
} t_stats;

/* Tout l'état de la simulation */
typedef struct {
    t_case grille[NB_LIGNES][NB_COLONNES];
    t_dechet dechets[MAX_DECHETS];
    int nb_dechets;               /* cases utilisées dans le tableau dechets */
    t_engin drone;
    t_engin robot;
    t_type_engin engin_actif;
    t_stats stats;
    int en_cours;                 /* passe à 0 quand on quitte */
    char message[TAILLE_MESSAGE]; /* dernier message, rempli par le moteur */
} t_simulation;

/* ---------- Sous-programmes ---------- */

/* Initialisation */
void init_simulation(t_simulation *sim);

/* Tests sur les cases */
int est_dans_grille(int lig, int col);
int est_obstacle(t_case terrain);
int case_franchissable(const t_simulation *sim, int lig, int col);
int indice_dechet_en(const t_simulation *sim, int lig, int col);
int dechet_visible_en(const t_simulation *sim, int lig, int col);

/* Déplacement (commun au drone et au robot). Renvoie 1 si l'engin a bougé, 0 sinon. */
int deplacer_engin(t_simulation *sim, t_engin *engin, t_commande direction);

/* Règles */
int detecter_dechets(t_simulation *sim);
int ramasser_dechet(t_simulation *sim);
int deposer_dechet(t_simulation *sim);
int ajouter_dechet(t_simulation *sim, int lig, int col);
int recharger_si_station(t_simulation *sim, t_engin *engin);
int gerer_panne(t_simulation *sim, t_engin *engin);
int faire_apparaitre_dechet(t_simulation *sim);

/* Un tour complet : applique la commande puis fait évoluer la réserve */
void appliquer_commande(t_simulation *sim, t_commande cmd);

/* Nombre de déchets détectés et encore au sol */
int nb_dechets_en_attente(const t_simulation *sim);

#endif
