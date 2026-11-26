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
    printf("- Le robot ramasse un dechet detecte en passant dessus (un seul a la fois),\n");
    printf("  puis le depose en passant sur un centre de collecte.\n");
    printf("- Ni le drone ni le robot ne traversent les obstacles naturels :\n");
    printf("  forets denses, falaises et plans d'eau.\n\n");
    printf("COMMANDES\n\n");
    printf("  Fleches ou Z Q S D : deplacer l'engin actif\n");
    printf("  Tab                : passer du drone au robot\n");
    printf("  Echap              : terminer la simulation\n\n");
    printf("LEGENDE\n\n");
    printf("  D drone   R robot   * dechet detecte   C centre de collecte   . libre\n");
    printf("  # foret dense   ^ falaise   ~ plan d'eau\n\n");
    printf("Appuyez sur une touche pour revenir au menu...");
    fflush(stdout);
}

/* Affiche une case en choisissant le symbole le plus important à montrer */
static void afficher_case(const t_simulation *sim, int lig, int col)
{
    int drone_ici = sim->drone.pos.lig == lig && sim->drone.pos.col == col;
    int robot_ici = sim->robot.pos.lig == lig && sim->robot.pos.col == col;

    /* Si les deux engins sont sur la même case, on montre celui qu'on pilote */
    if (drone_ici && (!robot_ici || sim->engin_actif == DRONE)) {
        changer_couleur(COULEUR_DRONE);
        printf("D ");
    } else if (robot_ici) {
        changer_couleur(COULEUR_ROBOT);
        printf("R ");
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
    } else if (sim->grille[lig][col] == CENTRE) {
        changer_couleur(COULEUR_CENTRE);
        printf("C ");
    } else {
        changer_couleur(COULEUR_LIBRE);
        printf(". ");
    }
    changer_couleur(COULEUR_DEFAUT);
}

/* Affiche la ligne numéro "ligne" du panneau d'infos, à droite de la grille */
static void afficher_ligne_panneau(const t_simulation *sim, int ligne)
{
    switch (ligne) {
        case 0:  printf("   Engin actif : %s", sim->engin_actif == DRONE ? "DRONE" : "ROBOT"); break;
        case 1:  printf("   Tour        : %d", sim->stats.nb_tours); break;
        case 3:  printf("   Detectes    : %d", sim->stats.nb_detectes); break;
        case 4:  printf("   En attente  : %d", nb_dechets_en_attente(sim)); break;
        case 5:  printf("   Collectes   : %d", sim->stats.nb_collectes); break;
        case 6:  printf("   Robot       : %s", sim->robot.charge ? "charge" : "vide"); break;
        case 8:  printf("   [Fleches/ZQSD] bouger"); break;
        case 9:  printf("   [Tab] changer d'engin"); break;
        case 10: printf("   [Echap] quitter"); break;
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
    printf("  Robot charge a la fin    : %s\n\n", sim->robot.charge ? "oui" : "non");
    printf("Appuyez sur une touche pour revenir au menu...");
    fflush(stdout);
}

void afficher_au_revoir(void)
{
    effacer_ecran();
    printf("Merci d'avoir protege la reserve. A bientot !\n");
}
