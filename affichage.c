/*
 * affichage.c — Tous les affichages du programme (partie IHM)
 *
 * Utilise les codes couleur ANSI, qui marchent dans le Terminal macOS, sous Linux
 * et dans la console Windows 10/11. Sous Windows, on pourra remplacer effacer_ecran()
 * et changer_couleur() par les fonctions du module affichage_console.h vu en cours,
 * sans rien changer d'autre dans le programme.
 *
 * Pendant la simulation, l'écran est redessiné 10 fois par seconde : on replace le curseur
 * en haut à gauche et on réécrit par-dessus (sans tout effacer), pour éviter le clignotement.
 */

#include <stdio.h>
#include "affichage.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define LARGEUR_CASE 2   /* chaque case occupe 2 colonnes à l'écran */

/* Codes couleur ANSI */
#define DEFAUT          "\033[0m"
#define C_DRONE         "\033[1;97;44m"   /* blanc sur bleu    */
#define C_ROBOT         "\033[1;30;47m"   /* noir sur blanc    */
#define C_PANNE         "\033[1;97;41m"   /* blanc sur rouge   */
#define C_RECHARGE      "\033[1;30;42m"   /* noir sur vert     */
#define C_TECH          "\033[1;97m"      /* blanc gras        */
#define C_MAISON        "\033[1;97;100m"  /* blanc sur gris    */
#define C_FORET         "\033[32m"        /* vert              */
#define C_FALAISE       "\033[37m"        /* gris clair        */
#define C_EAU           "\033[34m"        /* bleu              */
#define C_STATION       "\033[1;92m"      /* vert vif          */
#define C_LIBRE         "\033[90m"        /* gris foncé        */
#define C_DESTINATION   "\033[1;96m"      /* cyan vif          */
#define C_ALERTE        "\033[1;31m"      /* rouge             */
#define FIN_LIGNE       "\033[K\n"        /* efface la fin de la ligne puis passe à la suivante */

/* Couleurs par type de déchet : lettre du déchet, puis centre de tri */
static const char *C_DECHET[NB_TYPES_DECHETS] = {"\033[1;93m", "\033[1;96m", "\033[1;95m"};
static const char *C_CENTRE[NB_TYPES_DECHETS] = {"\033[1;30;43m", "\033[1;30;46m", "\033[1;30;45m"};
static const char LETTRE_DECHET[NB_TYPES_DECHETS] = {'p', 'v', 'm'};
static const char LETTRE_CENTRE[NB_TYPES_DECHETS] = {'P', 'V', 'M'};
static const char *NOM_TYPE[NB_TYPES_DECHETS] = {"plastique", "verre", "metal"};

static void effacer_ecran(void)
{
    printf("\033[H\033[2J");
}

static void changer_couleur(const char *code)
{
    printf("%s", code);
}

/* Écrit un symbole en couleur suivi d'une espace (une case = 2 colonnes) */
static void symbole(const char *couleur, char c)
{
    printf("%s%c%s ", couleur, c, DEFAUT);
}

void affichage_initialiser(void)
{
    static char tampon[1 << 16];

    /* Gros tampon : chaque image est envoyée d'un bloc, donc sans scintillement */
    setvbuf(stdout, tampon, _IOFBF, sizeof(tampon));
#ifdef _WIN32
    {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(console, &mode)) {
            SetConsoleMode(console, mode | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
        }
    }
#endif
}

void afficher_menu(void)
{
    effacer_ecran();
    printf("=========================================\n");
    printf("   ECOBOTS - Protection d'une reserve\n");
    printf("=========================================\n\n");
    printf("  1. Nouvelle simulation\n");
    printf("  2. Regles et commandes\n");
    printf("  3. Quitter\n\n");
    printf("Votre choix : ");
    fflush(stdout);
}

void afficher_regles(void)
{
    effacer_ecran();
    printf("REGLES\n\n");
    printf("- Des dechets de 3 types (plastique, verre, metal) apparaissent au hasard, invisibles.\n");
    printf("- Le drone detecte un dechet s'il passe sur sa case ou sur une des 8 cases voisines.\n");
    printf("- Le robot ramasse les dechets detectes en passant dessus (%d au maximum),\n", CAPACITE_ROBOT);
    printf("  puis les trie : chaque centre (P, V, M) n'accepte qu'un type de dechet.\n");
    printf("- Batteries : le drone use %d %% par case ; le robot %d %% a vide, + %d %% par dechet porte.\n",
           CONSO_DRONE, CONSO_ROBOT_VIDE, CONSO_ROBOT_PAR_DECHET);
    printf("  Sur une station (+), un engin se recharge a 100 %% en %d secondes.\n",
           DUREE_RECHARGE / TICKS_PAR_SECONDE);
    printf("- Batterie vide = panne. Le technicien (@) sort de sa maison (H), va chercher l'engin,\n");
    printf("  le porte jusqu'a la station la plus proche, puis rentre chez lui.\n\n");
    printf("COMMANDES\n\n");
    printf("  Clic gauche        : envoyer l'engin pilote sur la case (plus court chemin)\n");
    printf("  Fleches ou Z Q S D : avancer d'une case\n");
    printf("  Tab                : passer du drone au robot\n");
    printf("  Echap              : terminer la simulation\n\n");
    printf("Agrandissez la fenetre a au moins 125 colonnes x 23 lignes.\n\n");
    printf("Appuyez sur une touche pour revenir au menu...");
    fflush(stdout);
}

void preparer_ecran_simulation(void)
{
    effacer_ecran();
    fflush(stdout);
}

int ecran_vers_case(int x, int y, t_position *p)
{
    if (p == NULL || !est_dans_grille(y, x / LARGEUR_CASE)) {
        return 0;
    }
    p->lig = y;
    p->col = x / LARGEUR_CASE;
    return 1;
}

/* Affiche une case en choisissant ce qu'il y a de plus important à montrer */
static void afficher_case(const t_simulation *sim, int lig, int col)
{
    const t_technicien *tech = &sim->technicien;
    const t_engin *pilote = (sim->engin_actif == DRONE) ? &sim->drone : &sim->robot;
    t_position ici = {lig, col};
    t_case terrain = sim->grille[lig][col];
    int i;

#define ICI(p) ((p).lig == ici.lig && (p).col == ici.col)

    if (tech->etat != A_LA_MAISON && ICI(tech->pos)) {
        symbole(tech->etat == RAMENE ? C_PANNE : C_TECH, '@');
    } else if (ICI(sim->drone.pos) && (!ICI(sim->robot.pos) || sim->engin_actif == DRONE)) {
        symbole(sim->drone.etat == OPERATIONNEL ? C_DRONE
                : sim->drone.etat == EN_RECHARGE ? C_RECHARGE : C_PANNE, 'D');
    } else if (ICI(sim->robot.pos)) {
        symbole(sim->robot.etat == OPERATIONNEL ? C_ROBOT
                : sim->robot.etat == EN_RECHARGE ? C_RECHARGE : C_PANNE, 'R');
    } else if (pilote->a_destination && ICI(pilote->destination)) {
        symbole(C_DESTINATION, 'x');
    } else if ((i = indice_dechet_en(sim, lig, col)) != -1 && sim->dechets[i].detecte) {
        symbole(C_DECHET[sim->dechets[i].type], LETTRE_DECHET[sim->dechets[i].type]);
    } else if (est_centre(terrain)) {
        symbole(C_CENTRE[type_du_centre(terrain)], LETTRE_CENTRE[type_du_centre(terrain)]);
    } else {
        switch (terrain) {
            case FORET:   symbole(C_FORET, '#'); break;
            case FALAISE: symbole(C_FALAISE, '^'); break;
            case EAU:     symbole(C_EAU, '~'); break;
            case STATION: symbole(C_STATION, '+'); break;
            case MAISON:  symbole(C_MAISON, 'H'); break;
            default:      symbole(C_LIBRE, '.'); break;
        }
    }
#undef ICI
}

/* Jauge de batterie de 10 traits, rouge sous le seuil d'alerte */
static void afficher_jauge(int batterie)
{
    int i, pleins = (batterie + 9) / 10;

    printf("[");
    changer_couleur(batterie <= SEUIL_BATTERIE_FAIBLE ? C_ALERTE : C_STATION);
    for (i = 0; i < 10; i++) {
        printf(i < pleins ? "|" : " ");
    }
    printf("%s] %3d %%", DEFAUT, batterie);
}

static void afficher_etat_engin(const t_engin *engin)
{
    switch (engin->etat) {
        case OPERATIONNEL:
            afficher_jauge(engin->batterie);
            break;
        case EN_RECHARGE:
            afficher_jauge(engin->batterie);
            printf(" %srecharge %d s%s", C_STATION,
                   (engin->temps_recharge + TICKS_PAR_SECONDE - 1) / TICKS_PAR_SECONDE, DEFAUT);
            break;
        case EN_PANNE:
            printf("%sEN PANNE%s - attend le technicien", C_ALERTE, DEFAUT);
            break;
        default:
            printf("%sEN PANNE%s - porte par le technicien", C_ALERTE, DEFAUT);
            break;
    }
}

static void afficher_etat_technicien(const t_simulation *sim)
{
    const t_technicien *tech = &sim->technicien;
    const char *engin = (tech->cible == DRONE) ? "drone" : "robot";

    switch (tech->etat) {
        case A_LA_MAISON:
            printf("a la maison");
            break;
        case VA_CHERCHER:
            printf("va chercher le %s", engin);
            break;
        case RAMENE:
            printf("porte le %s (%d cases)", engin, distance_trajet(sim, tech->pos, tech->station, 1));
            break;
        default:
            printf("rentre chez lui (%d cases)", distance_trajet(sim, tech->pos, tech->maison, 1));
            break;
    }
}

/* Ligne "type de déchet" du tableau de statistiques */
static void afficher_ligne_type(const t_simulation *sim, t_type_dechet t)
{
    printf("%s%c%s %-9s  %8d  %5d  %6d", C_DECHET[t], LETTRE_DECHET[t], DEFAUT, NOM_TYPE[t],
           sim->stats.nb_detectes[t], sim->stats.nb_collectes[t], nb_dechets_en_attente_type(sim, t));
}

/* Affiche la ligne numéro "ligne" du panneau d'infos, à droite de la grille */
static void afficher_ligne_panneau(const t_simulation *sim, int ligne)
{
    int secondes = sim->stats.nb_ticks / TICKS_PAR_SECONDE;

    printf("  ");
    switch (ligne) {
        case 0:
            printf("Temps %02d:%02d    Pilote : %s", secondes / 60, secondes % 60,
                   sim->engin_actif == DRONE ? "DRONE" : "ROBOT");
            break;
        case 2:  printf("Drone  "); afficher_etat_engin(&sim->drone); break;
        case 3:  printf("Robot  "); afficher_etat_engin(&sim->robot); break;
        case 4:  printf("Technicien : "); afficher_etat_technicien(sim); break;
        case 6:
            printf("Robot porte : %sp%s %d  %sv%s %d  %sm%s %d   (%d/%d)",
                   C_DECHET[PLASTIQUE], DEFAUT, sim->robot.charge[PLASTIQUE],
                   C_DECHET[VERRE], DEFAUT, sim->robot.charge[VERRE],
                   C_DECHET[METAL], DEFAUT, sim->robot.charge[METAL],
                   charge_totale(&sim->robot), CAPACITE_ROBOT);
            break;
        case 8:  printf("             detectes  tries  au sol"); break;
        case 9:  afficher_ligne_type(sim, PLASTIQUE); break;
        case 10: afficher_ligne_type(sim, VERRE); break;
        case 11: afficher_ligne_type(sim, METAL); break;
        case 12: printf("Pannes : %d   Recharges : %d", sim->stats.nb_pannes, sim->stats.nb_recharges); break;
        case 14: printf("Clic gauche : y envoyer l'engin"); break;
        case 15: printf("Fleches/ZQSD : avancer d'une case"); break;
        case 16: printf("Tab : changer d'engin   Echap : quitter"); break;
        case 18:
            printf("%sP%s %sV%s %sM%s centres de tri   %s+%s station",
                   C_CENTRE[PLASTIQUE], DEFAUT, C_CENTRE[VERRE], DEFAUT, C_CENTRE[METAL], DEFAUT,
                   C_STATION, DEFAUT);
            break;
        case 19:
            printf("%sH%s maison  %s@%s technicien  %s#%s foret %s^%s falaise %s~%s eau",
                   C_MAISON, DEFAUT, C_TECH, DEFAUT, C_FORET, DEFAUT, C_FALAISE, DEFAUT, C_EAU, DEFAUT);
            break;
        default:
            break;
    }
}

void afficher_simulation(const t_simulation *sim)
{
    int lig, col;

    if (sim == NULL) {
        return;
    }

    printf("\033[H");   /* curseur en haut à gauche : on dessine par-dessus l'image précédente */
    for (lig = 0; lig < NB_LIGNES; lig++) {
        for (col = 0; col < NB_COLONNES; col++) {
            afficher_case(sim, lig, col);
        }
        afficher_ligne_panneau(sim, lig);
        printf(FIN_LIGNE);
    }
    printf(FIN_LIGNE);
    printf("> %s" FIN_LIGNE, sim->message);
    fflush(stdout);
}

void afficher_bilan(const t_simulation *sim)
{
    int t, secondes;

    if (sim == NULL) {
        return;
    }
    secondes = sim->stats.nb_ticks / TICKS_PAR_SECONDE;

    effacer_ecran();
    printf("=========== BILAN DE LA SIMULATION ===========\n\n");
    printf("  Duree                    : %02d:%02d\n", secondes / 60, secondes % 60);
    printf("  Dechets apparus          : %d\n\n", sim->stats.nb_apparus);
    printf("               detectes   tries   restes au sol\n");
    for (t = 0; t < NB_TYPES_DECHETS; t++) {
        printf("  %-10s   %8d   %5d   %13d\n", NOM_TYPE[t], sim->stats.nb_detectes[t],
               sim->stats.nb_collectes[t], nb_dechets_en_attente_type(sim, (t_type_dechet) t));
    }
    printf("\n  Recharges                : %d\n", sim->stats.nb_recharges);
    printf("  Pannes de batterie       : %d\n", sim->stats.nb_pannes);
    printf("  Dechets dans le robot    : %d\n\n", charge_totale(&sim->robot));
    printf("Appuyez sur une touche pour revenir au menu...");
    fflush(stdout);
}

void afficher_au_revoir(void)
{
    effacer_ecran();
    printf("Merci d'avoir protege la reserve. A bientot !\n");
    fflush(stdout);
}
