/*
 * simulation.h — Moteur de la simulation EcoBots
 *
 * Ce module contient TOUTES les données et TOUTES les règles de la simulation.
 * Il ne fait AUCUN affichage (pas de printf) et AUCUNE saisie clavier (pas de getch) :
 * on peut changer l'IHM (console, raylib...) sans modifier ce module.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

/* ---------- Constantes : réserve ---------- */

#define NB_LIGNES          20   /* hauteur de la grille */
#define NB_COLONNES        40   /* largeur de la grille */
#define MAX_DECHETS        80   /* nombre max de déchets en même temps */
#define PROBA_APPARITION   10   /* chance (en %) qu'un déchet apparaisse à chaque tour */
#define MAX_ESSAIS_CASE   100   /* essais max pour trouver une case libre au hasard */
#define TAILLE_MESSAGE    100   /* taille du message affiché par l'IHM */

/* ---------- Constantes : engins et batteries (en %) ---------- */

#define CAPACITE_ROBOT          3   /* déchets transportés par le robot à la fois */
#define BATTERIE_MAX          100   /* batterie pleine */
#define CONSO_DRONE             2   /* le drone vole : 2 % par case */
#define CONSO_ROBOT_VIDE        1   /* le robot roule à vide : 1 % par case */
#define CONSO_ROBOT_PAR_DECHET  1   /* + 1 % par case pour chaque déchet transporté */
#define RECHARGE_PAR_TOUR      25   /* % regagnés à chaque tour passé sur une station */
#define SEUIL_BATTERIE_FAIBLE  20   /* en dessous : alerte "batterie faible" */

/* ---------- Types ---------- */

/* Nature du terrain d'une case.
 * FORET, FALAISE et EAU sont des obstacles naturels infranchissables. */
typedef enum {
    LIBRE,
    FORET,
    FALAISE,
    EAU,
    CENTRE,
    STATION    /* station de recharge, d'où partent aussi les techniciens */
} t_case;

/* Commandes possibles, indépendantes des touches du clavier */
typedef enum {
    AUCUNE,
    HAUT,
    BAS,
    GAUCHE,
    DROITE,
    ATTENDRE,        /* laisser passer un tour (recharge, attente du technicien) */
    CHANGER_ENGIN,
    QUITTER
} t_commande;

/* Engin actuellement piloté */
typedef enum {
    DRONE,
    ROBOT
} t_type_engin;

/* État d'un engin */
typedef enum {
    OPERATIONNEL,    /* pilotable */
    EN_PANNE,        /* batterie vide : un technicien marche vers lui */
    REMORQUE         /* le technicien le ramène à pied jusqu'à la station */
} t_etat_engin;

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
    int charge;              /* robot seulement : déchets transportés (0 à CAPACITE_ROBOT) */
    int batterie;            /* en %, de 0 à BATTERIE_MAX */
    t_etat_engin etat;
    t_position technicien;   /* position du technicien envoyé en cas de panne */
    t_position station;      /* station d'où vient le technicien et où il ramène l'engin */
} t_engin;

/* Compteurs pour le panneau d'infos et le bilan */
typedef struct {
    int nb_apparus;
    int nb_detectes;
    int nb_collectes;
    int nb_tours;
    int nb_pannes;
    int nb_tours_immobilises;   /* tours passés en panne ou remorqué, tous engins confondus */
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

/* Chemins à pied (parcours en largeur) : -1 si la case est inaccessible */
int distance_trajet(const t_simulation *sim, t_position depart, t_position arrivee);
t_position prochain_pas(const t_simulation *sim, t_position depart, t_position arrivee);

/* Déplacement (commun au drone et au robot). Renvoie 1 si l'engin a bougé, 0 sinon. */
int deplacer_engin(t_simulation *sim, t_engin *engin, t_commande direction);

/* Règles sur les déchets */
int detecter_dechets(t_simulation *sim);
int ramasser_dechet(t_simulation *sim);
int deposer_dechet(t_simulation *sim);
int ajouter_dechet(t_simulation *sim, int lig, int col);
int faire_apparaitre_dechet(t_simulation *sim);

/* Règles sur les batteries et les pannes */
int consommation_deplacement(const t_simulation *sim, const t_engin *engin);
int recharger_si_station(t_simulation *sim, t_engin *engin);
void declencher_panne(t_simulation *sim, t_engin *engin);
void avancer_secours(t_simulation *sim, t_engin *engin);

/* Un tour complet : applique la commande puis fait évoluer la réserve */
void appliquer_commande(t_simulation *sim, t_commande cmd);

/* Informations pour l'affichage */
int nb_dechets_en_attente(const t_simulation *sim);
const char *nom_engin(const t_simulation *sim, const t_engin *engin);

#endif
