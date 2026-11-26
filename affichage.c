/*
 * affichage.c — Tous les affichages du programme (partie IHM)
 *
 * Utilise les codes couleur ANSI, qui marchent dans le Terminal macOS, sous Linux
 * et dans la console Windows 10/11. Sous Windows, on pourra remplacer effacer_ecran()
 * et changer_couleur() par les fonctions du module affichage_console.h vu en cours,
 * sans rien changer d'autre dans le programme.
 */

#include <stdio.h>
#include "affichage.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* Codes couleur ANSI */
#define COULEUR_DEFAUT  "\033[0m"
#define COULEUR_DRONE   "\033[1;36m"   /* cyan gras   */
#define COULEUR_ROBOT   "\033[1;33m"   /* jaune gras  */
#define COULEUR_DECHET  "\033[1;31m"   /* rouge gras  */
#define COULEUR_FORET   "\033[32m"    /* vert        */
#define COULEUR_FALAISE "\033[37m"    /* gris clair  */
#define COULEUR_EAU     "\033[1;34m"  /* bleu gras   */
#define COULEUR_CENTRE  "\033[1;35m"   /* magenta gras */
#define COULEUR_LIBRE   "\033[90m"     /* gris        */
#define COULEUR_STATION "\033[1;92m"  /* vert vif gras */
#define COULEUR_PANNE   "\033[1;37;41m" /* blanc sur fond rouge */
#define COULEUR_TECH    "\033[1;97m"  /* blanc gras   */
#define COULEUR_ALERTE  "\033[1;31m"   /* rouge gras  */

static void effacer_ecran(void)
{
    printf("\033[H\033[2J");
}

static void changer_couleur(const char *code)
{
    printf("%s", code);
}

void affichage_initialiser(void)
{
#ifdef _WIN32
    /* Active les codes couleur ANSI dans la console Windows */
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(console, &mode)) {
        SetConsoleMode(console, mode | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
    }
    SetConsoleOutputCP(65001);   /* UTF-8 */
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
    printf("- Des dechets apparaissent au hasard dans la reserve, mais restent invisibles.\n");
    printf("- Le drone detecte un dechet s'il passe sur sa case ou sur une des 8 cases voisines.\n");
    printf("- Le robot ramasse un dechet detecte en passant dessus (jusqu'a %d a la fois),\n", CAPACITE_ROBOT);
    printf("  puis les vide tous en passant sur un centre de collecte.\n");
    printf("- Batteries : le drone use %d %% par case, le robot %d %% a vide et %d %% de plus\n",
           CONSO_DRONE, CONSO_ROBOT_VIDE, CONSO_ROBOT_PAR_DECHET);
    printf("  par dechet transporte. Sur une station (+), chaque tour rend %d %% (Espace pour attendre).\n",
           RECHARGE_PAR_TOUR);
    printf("- Batterie vide = panne : l'engin s'arrete. Un technicien (H) part a pied de la station\n");
    printf("  la plus proche, vient le chercher puis le ramene a la station. Pendant ce temps,\n");
    printf("  pilotez l'autre engin (Tab) ou attendez (Espace).\n");
    printf("- Ni le drone ni le robot ne traversent les obstacles naturels :\n");
    printf("  forets denses, falaises et plans d'eau.\n\n");
    printf("COMMANDES\n\n");
    printf("  Fleches ou Z Q S D : deplacer l'engin actif\n");
    printf("  Espace             : attendre un tour (recharge, technicien)\n");
    printf("  Tab                : passer du drone au robot\n");
    printf("  Echap              : terminer la simulation\n\n");
    printf("LEGENDE\n\n");
    printf("  D drone   R robot   H technicien   * dechet detecte   C centre de collecte\n");
    printf("  + station de recharge   . libre   (engin sur fond rouge = en panne)\n");
    printf("  # foret dense   ^ falaise   ~ plan d'eau\n\n");
    printf("Appuyez sur une touche pour revenir au menu...");
    fflush(stdout);
}

/* Affiche une case en choisissant le symbole le plus important à montrer */
static void afficher_case(const t_simulation *sim, int lig, int col)
{
    int drone_ici = sim->drone.pos.lig == lig && sim->drone.pos.col == col;
    int robot_ici = sim->robot.pos.lig == lig && sim->robot.pos.col == col;

    int tech_ici = (sim->drone.etat == EN_PANNE && sim->drone.technicien.lig == lig
                    && sim->drone.technicien.col == col)
                || (sim->robot.etat == EN_PANNE && sim->robot.technicien.lig == lig
                    && sim->robot.technicien.col == col);

    /* Si les deux engins sont sur la même case, on montre celui qu'on pilote */
    if (drone_ici && (!robot_ici || sim->engin_actif == DRONE)) {
        changer_couleur(sim->drone.etat == OPERATIONNEL ? COULEUR_DRONE : COULEUR_PANNE);
        printf("D");
        changer_couleur(COULEUR_DEFAUT);
        printf(" ");
    } else if (robot_ici) {
        changer_couleur(sim->robot.etat == OPERATIONNEL ? COULEUR_ROBOT : COULEUR_PANNE);
        printf("R");
        changer_couleur(COULEUR_DEFAUT);
        printf(" ");
    } else if (tech_ici) {
        changer_couleur(COULEUR_TECH);
        printf("H ");
    } else if (dechet_visible_en(sim, lig, col)) {
        changer_couleur(COULEUR_DECHET);
        printf("* ");
    } else if (sim->grille[lig][col] == FORET) {
        changer_couleur(COULEUR_FORET);
        printf("# ");
    } else if (sim->grille[lig][col] == FALAISE) {
        changer_couleur(COULEUR_FALAISE);
        printf("^ ");
    } else if (sim->grille[lig][col] == EAU) {
        changer_couleur(COULEUR_EAU);
        printf("~ ");
    } else if (sim->grille[lig][col] == STATION) {
        changer_couleur(COULEUR_STATION);
        printf("+ ");
    } else if (sim->grille[lig][col] == CENTRE) {
        changer_couleur(COULEUR_CENTRE);
        printf("C ");
    } else {
        changer_couleur(COULEUR_LIBRE);
        printf(". ");
    }
    changer_couleur(COULEUR_DEFAUT);
}

/* Dessine l'état d'un engin : jauge de batterie (rouge sous le seuil) ou état de la panne */
static void afficher_etat_engin(const t_simulation *sim, const t_engin *engin)
{
    int i;
    int pleins = (engin->batterie + 9) / 10;   /* 10 carrés, arrondi au-dessus */

    if (engin->etat == EN_PANNE) {
        changer_couleur(COULEUR_ALERTE);
        printf("EN PANNE");
        changer_couleur(COULEUR_DEFAUT);
        printf(" - technicien a %d cases", distance_trajet(sim, engin->technicien, engin->pos));
        return;
    }
    if (engin->etat == REMORQUE) {
        changer_couleur(COULEUR_ALERTE);
        printf("REMORQUE");
        changer_couleur(COULEUR_DEFAUT);
        printf(" - station a %d cases", distance_trajet(sim, engin->pos, engin->station));
        return;
    }

    printf("[");
    changer_couleur(engin->batterie <= SEUIL_BATTERIE_FAIBLE ? COULEUR_ALERTE : COULEUR_STATION);
    for (i = 0; i < 10; i++) {
        printf(i < pleins ? "|" : " ");
    }
    changer_couleur(COULEUR_DEFAUT);
    printf("] %3d %%", engin->batterie);
}

/* Affiche la ligne numéro "ligne" du panneau d'infos, à droite de la grille */
static void afficher_ligne_panneau(const t_simulation *sim, int ligne)
{
    switch (ligne) {
        case 0:  printf("   Engin actif : %s", sim->engin_actif == DRONE ? "DRONE" : "ROBOT"); break;
        case 1:  printf("   Tour        : %d", sim->stats.nb_tours); break;
        case 3:  printf("   Drone "); afficher_etat_engin(sim, &sim->drone); break;
        case 4:  printf("   Robot "); afficher_etat_engin(sim, &sim->robot); break;
        case 6:  printf("   Detectes    : %d", sim->stats.nb_detectes); break;
        case 7:  printf("   En attente  : %d", nb_dechets_en_attente(sim)); break;
        case 8:  printf("   Collectes   : %d", sim->stats.nb_collectes); break;
        case 9:  printf("   Robot       : %d/%d dechets", sim->robot.charge, CAPACITE_ROBOT); break;
        case 10: printf("   Pannes      : %d", sim->stats.nb_pannes); break;
        case 12: printf("   [Fleches/ZQSD] bouger"); break;
        case 13: printf("   [Espace] attendre un tour"); break;
        case 14: printf("   [Tab] changer d'engin"); break;
        case 15: printf("   [Echap] quitter"); break;
        case 17: printf("   + recharge  C collecte  H technicien"); break;
        case 18: printf("   # foret  ^ falaise  ~ eau"); break;
        default: break;
    }
}

void afficher_simulation(const t_simulation *sim)
{
    int lig, col;

    if (sim == NULL) {
        return;
    }

    effacer_ecran();
    for (lig = 0; lig < NB_LIGNES; lig++) {
        for (col = 0; col < NB_COLONNES; col++) {
            afficher_case(sim, lig, col);
        }
        afficher_ligne_panneau(sim, lig);
        printf("\n");
    }
    printf("\n> %s\n", sim->message);
    fflush(stdout);
}

void afficher_bilan(const t_simulation *sim)
{
    if (sim == NULL) {
        return;
    }

    effacer_ecran();
    printf("=========== BILAN DE LA SIMULATION ===========\n\n");
    printf("  Tours joues              : %d\n", sim->stats.nb_tours);
    printf("  Dechets apparus          : %d\n", sim->stats.nb_apparus);
    printf("  Dechets detectes         : %d\n", sim->stats.nb_detectes);
    printf("  Dechets rapportes        : %d\n", sim->stats.nb_collectes);
    printf("  Detectes non ramasses    : %d\n", nb_dechets_en_attente(sim));
    printf("  Pannes de batterie       : %d\n", sim->stats.nb_pannes);
    printf("  Tours d'immobilisation   : %d\n", sim->stats.nb_tours_immobilises);
    printf("  Dechets dans le robot    : %d\n\n", sim->robot.charge);
    printf("Appuyez sur une touche pour revenir au menu...");
    fflush(stdout);
}

void afficher_au_revoir(void)
{
    effacer_ecran();
    printf("Merci d'avoir protege la reserve. A bientot !\n");
}
