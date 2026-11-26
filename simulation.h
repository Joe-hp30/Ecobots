/*
 * simulation.h — Moteur de la simulation EcoBots
 *
 * Ce module contient TOUTES les données et TOUTES les règles de la simulation.
 * Il ne fait AUCUN affichage (pas de printf) et AUCUNE saisie clavier (pas de getch) :
 * on peut changer l'IHM (console, raylib...) sans modifier ce module.
 *
 * La simulation avance en temps réel par petits pas appelés "ticks" :
 * le programme principal appelle avancer_temps() TICKS_PAR_SECONDE fois par seconde.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

/* ---------- Constantes : réserve et temps ---------- */

#define NB_LIGNES            20   /* hauteur de la grille */
#define NB_COLONNES          40   /* largeur de la grille */
#define MAX_DECHETS          80   /* nombre max de déchets en même temps */
#define MAX_ESSAIS_CASE     100   /* essais max pour trouver une case libre au hasard */
#define TAILLE_MESSAGE      100   /* taille du message affiché par l'IHM */

#define TICKS_PAR_SECONDE    10   /* un tick = 0,1 seconde */
#define PROBA_APPARITION     15   /* chance (pour mille) qu'un déchet apparaisse à chaque tick */
#define TICKS_PAR_CASE_ENGIN  2   /* en trajet automatique, un engin avance de 5 cases par seconde */
#define TICKS_PAR_CASE_PIETON 3   /* le technicien marche un peu moins vite */

/* ---------- Constantes : engins et batteries (en %) ---------- */

#define CAPACITE_ROBOT          3   /* déchets transportés par le robot à la fois, tous types confondus */
#define BATTERIE_MAX          100   /* batterie pleine */
#define CONSO_DRONE             2   /* le drone vole : 2 % par case */
#define CONSO_ROBOT_VIDE        1   /* le robot roule à vide : 1 % par case */
#define CONSO_ROBOT_PAR_DECHET  1   /* + 1 % par case pour chaque déchet transporté */
#define DUREE_RECHARGE         (5 * TICKS_PAR_SECONDE)   /* une recharge dure 5 secondes */
#define SEUIL_BATTERIE_FAIBLE  20   /* en dessous : alerte "batterie faible" */

/* ---------- Types ---------- */

/* Les types de déchets, triés dans des centres différents */
typedef enum {
    PLASTIQUE,
    VERRE,
    METAL,
    NB_TYPES_DECHETS
} t_type_dechet;

/* Nature du terrain d'une case.
 * FORET, FALAISE et EAU sont des obstacles naturels infranchissables.
 * La MAISON du technicien est accessible à pied, mais pas aux engins. */
typedef enum {
    LIBRE,
    FORET,
    FALAISE,
    EAU,
    CENTRE_PLASTIQUE,
    CENTRE_VERRE,
    CENTRE_METAL,
    STATION,
    MAISON
} t_case;

/* Commandes possibles, indépendantes des touches du clavier et de la souris */
typedef enum {
    AUCUNE,
    HAUT,
    BAS,
    GAUCHE,
    DROITE,
    CLIC,            /* clic sur une case : l'IHM fournit la case à part */
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
    EN_RECHARGE,     /* immobilisé sur une station pendant DUREE_RECHARGE */
    EN_PANNE,        /* batterie vide : attend le technicien */
    TRANSPORTE       /* porté par le technicien jusqu'à une station */
} t_etat_engin;

/* État du technicien */
typedef enum {
    A_LA_MAISON,
    VA_CHERCHER,     /* marche vers l'engin en panne */
    RAMENE,          /* porte l'engin jusqu'à la station la plus proche */
    RENTRE           /* rentre chez lui */
} t_etat_technicien;

/* Coordonnées d'une case dans la grille */
typedef struct {
    int lig;
    int col;
} t_position;

/* Un déchet de la réserve */
typedef struct {
    t_position pos;
    t_type_dechet type;
    int detecte;   /* 1 si le drone l'a repéré, 0 sinon */
    int actif;     /* 1 s'il est encore au sol, 0 s'il a été ramassé */
} t_dechet;

/* Le drone ou le robot */
typedef struct {
    t_position pos;
    int charge[NB_TYPES_DECHETS];  /* robot seulement : déchets transportés, par type */
    int batterie;                  /* en %, de 0 à BATTERIE_MAX */
    t_etat_engin etat;
    int a_destination;             /* 1 si un trajet automatique est en cours (clic) */
    t_position destination;
    int attente_pas;               /* ticks avant le prochain pas automatique */
    int temps_recharge;            /* ticks de recharge restants */
    int batterie_debut_recharge;   /* niveau au début de la recharge */
} t_engin;

/* Le technicien : un seul pour toute la réserve */
typedef struct {
    t_position pos;
    t_position maison;
    t_etat_technicien etat;
    t_type_engin cible;            /* engin qu'il va chercher ou qu'il ramène */
    t_position station;            /* station où il dépose l'engin */
    int attente_pas;
} t_technicien;

/* Compteurs pour le panneau d'infos et le bilan */
typedef struct {
    int nb_apparus;
    int nb_detectes[NB_TYPES_DECHETS];
    int nb_collectes[NB_TYPES_DECHETS];
    int nb_ticks;
    int nb_pannes;
    int nb_recharges;
} t_stats;

/* Tout l'état de la simulation */
typedef struct {
    t_case grille[NB_LIGNES][NB_COLONNES];
    t_dechet dechets[MAX_DECHETS];
    int nb_dechets;               /* cases utilisées dans le tableau dechets */
    t_engin drone;
    t_engin robot;
    t_technicien technicien;
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
int est_centre(t_case terrain);
t_type_dechet type_du_centre(t_case terrain);
int case_franchissable(const t_simulation *sim, int lig, int col);   /* pour les engins */
int case_praticable_a_pied(const t_simulation *sim, int lig, int col); /* pour le technicien */
int indice_dechet_en(const t_simulation *sim, int lig, int col);
int dechet_visible_en(const t_simulation *sim, int lig, int col);

/* Plus courts chemins (parcours en largeur) : -1 si la case est inaccessible.
 * a_pied = 1 pour le technicien, 0 pour un engin. */
int distance_trajet(const t_simulation *sim, t_position depart, t_position arrivee, int a_pied);
t_position prochain_pas(const t_simulation *sim, t_position depart, t_position arrivee, int a_pied);

/* Engins */
t_engin *engin_pilote(t_simulation *sim);
t_engin *engin_de_type(t_simulation *sim, t_type_engin type);
const char *nom_engin(const t_simulation *sim, const t_engin *engin);
int charge_totale(const t_engin *engin);
int deplacer_engin(t_simulation *sim, t_engin *engin, t_commande direction);
int definir_destination(t_simulation *sim, int lig, int col);

/* Déchets */
int detecter_dechets(t_simulation *sim);
int ramasser_dechet(t_simulation *sim);
int deposer_dechets(t_simulation *sim);
int ajouter_dechet(t_simulation *sim, int lig, int col, t_type_dechet type);
int faire_apparaitre_dechet(t_simulation *sim);

/* Batteries, recharges et pannes */
int consommation_deplacement(const t_simulation *sim, const t_engin *engin);
void demarrer_recharge(t_simulation *sim, t_engin *engin);
void declencher_panne(t_simulation *sim, t_engin *engin);

/* Une commande de l'utilisateur (déplacement d'une case, changement d'engin, quitter) */
void appliquer_commande(t_simulation *sim, t_commande cmd);

/* Fait avancer la simulation d'un tick : trajets, recharges, technicien, nouveaux déchets */
void avancer_temps(t_simulation *sim);

/* Informations pour l'affichage */
int nb_dechets_en_attente(const t_simulation *sim);
int nb_dechets_en_attente_type(const t_simulation *sim, t_type_dechet type);

#endif
