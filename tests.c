/*
 * tests.c — Tests automatiques du moteur (plan de tests, section 8.2 du rapport)
 *
 * Le moteur ne dépend pas de l'IHM : on peut le tester sans clavier ni écran.
 * Compilation et lancement : make tests
 */

#include <stdio.h>
#include "simulation.h"

static int nb_reussis = 0;
static int nb_echecs = 0;

/* Affiche OK ou ECHEC pour une condition, avec le nom du test */
#define VERIFIER(condition, nom) \
    do { \
        if (condition) { nb_reussis++; printf("  OK     %s\n", nom); } \
        else           { nb_echecs++;  printf("  ECHEC  %s\n", nom); } \
    } while (0)

static void placer(t_engin *engin, int lig, int col)
{
    engin->pos.lig = lig;
    engin->pos.col = col;
}

static void test_deplacements(void)
{
    t_simulation sim;
    int i;

    printf("Deplacements (F4, F8)\n");
    init_simulation(&sim);

    placer(&sim.drone, 0, 5);
    for (i = 0; i < 5; i++) {
        deplacer_engin(&sim, &sim.drone, HAUT);
    }
    VERIFIER(sim.drone.pos.lig == 0 && sim.drone.pos.col == 5, "T2  le drone ne sort pas par le bord haut");

    placer(&sim.drone, 0, 7);   /* la case (1, 7) est un obstacle */
    VERIFIER(deplacer_engin(&sim, &sim.drone, BAS) == 0, "T3  deplacer_engin renvoie 0 contre un obstacle");
    VERIFIER(sim.drone.pos.lig == 0, "T3  le drone reste sur place");

    placer(&sim.robot, 4, 0);
    VERIFIER(deplacer_engin(&sim, &sim.robot, GAUCHE) == 0, "T3b le robot ne sort pas par le bord gauche");
    VERIFIER(deplacer_engin(&sim, &sim.robot, DROITE) == 1 && sim.robot.pos.col == 1, "T3c deplacement valide");
    VERIFIER(deplacer_engin(&sim, &sim.robot, AUCUNE) == 0, "T3d une commande qui n'est pas une direction ne bouge rien");
}

static void test_detection(void)
{
    t_simulation sim;

    printf("Detection (F6)\n");

    init_simulation(&sim);
    ajouter_dechet(&sim, 5, 10);
    placer(&sim.drone, 4, 9);
    VERIFIER(detecter_dechets(&sim) == 1 && dechet_visible_en(&sim, 5, 10), "T4  dechet detecte sur une case en diagonale");

    init_simulation(&sim);
    ajouter_dechet(&sim, 5, 10);
    placer(&sim.drone, 3, 10);
    VERIFIER(detecter_dechets(&sim) == 0 && !dechet_visible_en(&sim, 5, 10), "T5  dechet a 2 cases : pas detecte");

    init_simulation(&sim);
    ajouter_dechet(&sim, 0, 0);
    placer(&sim.drone, 0, 1);
    VERIFIER(detecter_dechets(&sim) == 1, "T6  dechet dans un coin detecte, sans lire hors de la grille");
    VERIFIER(detecter_dechets(&sim) == 0 && sim.stats.nb_detectes == 1, "T6b un dechet n'est compte qu'une fois");
}

static void test_collecte(void)
{
    t_simulation sim;

    printf("Ramassage et depot (F9, F10)\n");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10);
    sim.dechets[0].detecte = 1;
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 1 && sim.robot.charge == 1, "T7  robot vide sur un dechet detecte : ramasse");
    VERIFIER(indice_dechet_en(&sim, 4, 10) == -1, "T7b le dechet a disparu de la grille");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10);
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 0 && sim.robot.charge == 0, "T8  dechet non detecte : rien ne se passe");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10);
    sim.dechets[0].detecte = 1;
    sim.robot.charge = 1;
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 0 && indice_dechet_en(&sim, 4, 10) != -1, "T9  robot plein : il ne prend pas le 2e dechet");

    init_simulation(&sim);
    sim.robot.charge = 1;
    placer(&sim.robot, 1, 1);   /* (1, 1) est un centre de collecte */
    VERIFIER(deposer_dechet(&sim) == 1 && sim.robot.charge == 0 && sim.stats.nb_collectes == 1,
             "T10 robot charge sur un centre : depot, collectes + 1");

    init_simulation(&sim);
    sim.robot.charge = 1;
    placer(&sim.robot, 4, 4);
    VERIFIER(deposer_dechet(&sim) == 0 && sim.robot.charge == 1, "T10b pas de depot hors d'un centre");
}

static void test_apparition(void)
{
    t_simulation sim;
    int lig, col, ajoutes = 0;

    printf("Apparition des dechets (F5)\n");
    init_simulation(&sim);

    VERIFIER(ajouter_dechet(&sim, 1, 7) == 0, "T11a pas de dechet sur un obstacle");
    VERIFIER(ajouter_dechet(&sim, 1, 1) == 0, "T11b pas de dechet sur un centre ni sous un engin");
    VERIFIER(ajouter_dechet(&sim, -1, 3) == 0 && ajouter_dechet(&sim, 3, NB_COLONNES) == 0, "T11c pas de dechet hors grille");

    for (lig = 0; lig < NB_LIGNES; lig++) {
        for (col = 0; col < NB_COLONNES; col++) {
            ajoutes += ajouter_dechet(&sim, lig, col);
        }
    }
    VERIFIER(ajoutes == MAX_DECHETS && sim.nb_dechets == MAX_DECHETS, "T11 jamais plus de MAX_DECHETS dechets");
    VERIFIER(faire_apparaitre_dechet(&sim) == 0, "T11d tableau plein : pas d'apparition ni de boucle infinie");

    sim.dechets[0].actif = 0;   /* comme si le robot l'avait ramasse */
    VERIFIER(ajouter_dechet(&sim, 14, 29) == 1 && sim.nb_dechets == MAX_DECHETS, "T11e la place d'un dechet ramasse est reutilisee");
}

static void test_tour_complet(void)
{
    t_simulation sim;

    printf("Tour de simulation (F7, F12)\n");
    init_simulation(&sim);

    appliquer_commande(&sim, CHANGER_ENGIN);
    VERIFIER(sim.engin_actif == ROBOT && sim.stats.nb_tours == 1, "T13 Tab : on pilote le robot");

    appliquer_commande(&sim, AUCUNE);
    VERIFIER(sim.stats.nb_tours == 1, "T13b touche inutile : aucun tour joue");

    placer(&sim.robot, 0, 0);
    appliquer_commande(&sim, HAUT);
    VERIFIER(sim.stats.nb_tours == 1, "T13c deplacement refuse : aucun tour joue");

    /* Scenario complet : le drone detecte, le robot ramasse puis depose */
    init_simulation(&sim);              /* drone et robot sur le centre (1, 1) */
    ajouter_dechet(&sim, 3, 1);
    appliquer_commande(&sim, BAS);      /* drone en (2, 1) : voit (3, 1) */
    VERIFIER(dechet_visible_en(&sim, 3, 1), "S1  le drone detecte le dechet en passant a cote");
    appliquer_commande(&sim, CHANGER_ENGIN);
    appliquer_commande(&sim, BAS);
    appliquer_commande(&sim, BAS);      /* robot en (3, 1) */
    VERIFIER(sim.robot.charge == 1, "S2  le robot ramasse le dechet");
    appliquer_commande(&sim, HAUT);
    appliquer_commande(&sim, HAUT);     /* robot de retour en (1, 1) */
    VERIFIER(sim.robot.charge == 0 && sim.stats.nb_collectes == 1, "S3  le robot depose le dechet au centre");

    appliquer_commande(&sim, QUITTER);
    VERIFIER(sim.en_cours == 0, "T12 Echap : la simulation s'arrete");
}

int main(void)
{
    printf("===== Tests du moteur EcoBots =====\n\n");
    test_deplacements();
    test_detection();
    test_collecte();
    test_apparition();
    test_tour_complet();
    printf("\n%d test(s) reussi(s), %d echec(s)\n", nb_reussis, nb_echecs);
    return nb_echecs == 0 ? 0 : 1;
}
