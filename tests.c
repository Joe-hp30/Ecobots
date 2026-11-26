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

    VERIFIER(sim.grille[1][7] == FORET && sim.grille[1][21] == FALAISE && sim.grille[7][15] == EAU,
             "T1  les trois obstacles naturels sont bien charges");
    placer(&sim.drone, 0, 21);   /* falaise en (1, 21) */
    VERIFIER(deplacer_engin(&sim, &sim.drone, BAS) == 0, "T3e le drone ne traverse pas une falaise");
    placer(&sim.drone, 8, 14);   /* eau en (8, 15) */
    VERIFIER(deplacer_engin(&sim, &sim.drone, DROITE) == 0 && sim.drone.pos.col == 14, "T3f le drone ne traverse pas un plan d'eau");

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
    sim.robot.charge = CAPACITE_ROBOT;
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 0 && indice_dechet_en(&sim, 4, 10) != -1, "T9  robot plein : il ne prend pas un dechet de plus");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10);
    ajouter_dechet(&sim, 4, 11);
    sim.dechets[0].detecte = 1;
    sim.dechets[1].detecte = 1;
    sim.robot.charge = 1;
    placer(&sim.robot, 4, 10);
    ramasser_dechet(&sim);
    placer(&sim.robot, 4, 11);
    VERIFIER(ramasser_dechet(&sim) == 1 && sim.robot.charge == 3, "T9b le robot transporte jusqu'a 3 dechets");

    init_simulation(&sim);
    sim.robot.charge = 1;
    placer(&sim.robot, 1, 1);   /* (1, 1) est un centre de collecte */
    VERIFIER(deposer_dechet(&sim) == 1 && sim.robot.charge == 0 && sim.stats.nb_collectes == 1,
             "T10 robot charge sur un centre : depot, collectes + 1");

    init_simulation(&sim);
    sim.robot.charge = 1;
    placer(&sim.robot, 4, 4);
    VERIFIER(deposer_dechet(&sim) == 0 && sim.robot.charge == 1, "T10b pas de depot hors d'un centre");

    init_simulation(&sim);
    sim.robot.charge = 3;
    placer(&sim.robot, 7, 38);   /* autre centre de collecte */
    VERIFIER(deposer_dechet(&sim) == 3 && sim.robot.charge == 0 && sim.stats.nb_collectes == 3,
             "T10c le robot vide ses 3 dechets d'un coup, dans n'importe quel centre");
}

static void test_apparition(void)
{
    t_simulation sim;
    int lig, col, ajoutes = 0;

    printf("Apparition des dechets (F5)\n");
    init_simulation(&sim);

    VERIFIER(ajouter_dechet(&sim, 1, 7) == 0, "T11a pas de dechet dans une foret");
    VERIFIER(ajouter_dechet(&sim, 1, 21) == 0 && ajouter_dechet(&sim, 7, 15) == 0, "T11a pas de dechet sur une falaise ni dans l'eau");
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

static void test_batterie(void)
{
    t_simulation sim;
    t_position station = {2, 3};
    int tours, vu_remorque = 0, chemin_valide = 1;

    printf("Batterie et stations de recharge (E3)\n");
    init_simulation(&sim);
    VERIFIER(sim.grille[2][3] == STATION && sim.grille[17][38] == STATION, "B1  les stations de recharge sont chargees");
    VERIFIER(case_franchissable(&sim, 2, 3), "B1b un engin peut aller sur une station");
    VERIFIER(ajouter_dechet(&sim, 2, 3) == 0, "B1c pas de dechet sur une station");

    appliquer_commande(&sim, BAS);
    VERIFIER(sim.drone.batterie == BATTERIE_MAX - CONSO_DRONE, "B2  le drone use 2 % par case");

    sim.engin_actif = ROBOT;
    placer(&sim.robot, 4, 4);
    appliquer_commande(&sim, DROITE);
    VERIFIER(sim.robot.batterie == BATTERIE_MAX - 1, "B2b le robot vide use 1 % par case");
    sim.robot.charge = 3;
    sim.robot.batterie = 50;
    appliquer_commande(&sim, DROITE);
    VERIFIER(sim.robot.batterie == 46, "B2c le robot plein (3 dechets) use 4 % par case");

    init_simulation(&sim);
    sim.drone.batterie = 10;
    appliquer_commande(&sim, HAUT);    /* (0, 1) */
    placer(&sim.drone, 0, 0);
    appliquer_commande(&sim, HAUT);    /* bord : refuse */
    VERIFIER(sim.drone.batterie == 8, "B2d un deplacement refuse n'use pas la batterie");

    placer(&sim.drone, 2, 4);
    sim.drone.batterie = 10;
    appliquer_commande(&sim, GAUCHE);  /* arrive sur la station (2, 3) : 10 - 2 + 25 */
    VERIFIER(sim.drone.batterie == 33, "B3  recharge progressive : + 25 % par tour sur une station");
    appliquer_commande(&sim, ATTENDRE);
    VERIFIER(sim.drone.batterie == 58 && sim.stats.nb_tours == 3, "B3b Espace : un tour passe et la recharge continue");
    sim.drone.batterie = 90;
    appliquer_commande(&sim, ATTENDRE);
    VERIFIER(sim.drone.batterie == BATTERIE_MAX, "B3c la batterie ne depasse pas 100 %");

    /* Panne : un technicien vient a pied chercher le drone */
    init_simulation(&sim);
    placer(&sim.drone, 4, 4);
    sim.drone.batterie = 2;
    appliquer_commande(&sim, DROITE);  /* (4, 5), batterie a 0 */
    VERIFIER(sim.drone.etat == EN_PANNE && sim.stats.nb_pannes == 1, "B4  batterie vide : le drone tombe en panne");
    VERIFIER(sim.drone.pos.lig == 4 && sim.drone.pos.col == 5, "B4b le drone reste ou il est tombe (pas de teleportation)");
    VERIFIER(sim.drone.station.lig == 2 && sim.drone.station.col == 3, "B4c le technicien vient de la station la plus proche");

    appliquer_commande(&sim, DROITE);
    VERIFIER(sim.drone.pos.col == 5 && sim.stats.nb_tours == 1, "B5  un engin en panne ne se pilote plus");

    for (tours = 0; tours < 50 && sim.drone.etat != OPERATIONNEL; tours++) {
        appliquer_commande(&sim, ATTENDRE);
        if (sim.drone.etat == REMORQUE) {
            vu_remorque = 1;
        }
        if (!case_franchissable(&sim, sim.drone.technicien.lig, sim.drone.technicien.col)) {
            chemin_valide = 0;
        }
    }
    VERIFIER(chemin_valide, "B6a le technicien ne marche que sur des cases praticables");
    VERIFIER(vu_remorque, "B6  le technicien recupere le drone puis le remorque");
    VERIFIER(sim.drone.etat == OPERATIONNEL && sim.drone.pos.lig == station.lig && sim.drone.pos.col == station.col,
             "B6b le drone est ramene a la station");
    VERIFIER(tours >= 6 && sim.drone.batterie == BATTERIE_MAX, "B6c le sauvetage prend du temps, puis batterie pleine");

    init_simulation(&sim);
    sim.engin_actif = ROBOT;
    sim.robot.charge = 2;
    placer(&sim.robot, 12, 12);
    sim.robot.batterie = 1;
    appliquer_commande(&sim, BAS);
    VERIFIER(sim.robot.etat == EN_PANNE && sim.robot.charge == 2, "B7  le robot en panne garde ses dechets");
    appliquer_commande(&sim, CHANGER_ENGIN);
    appliquer_commande(&sim, DROITE);
    VERIFIER(sim.drone.pos.col == 2, "B7b on peut piloter le drone pendant la panne du robot");

    VERIFIER(distance_trajet(&sim, (t_position) {0, 0}, (t_position) {0, 5}) == 5, "B8  distance a pied en ligne droite");
    VERIFIER(distance_trajet(&sim, (t_position) {0, 7}, (t_position) {4, 8}) == 9, "B8b le chemin contourne la foret (9 pas au lieu de 5)");
    VERIFIER(distance_trajet(&sim, (t_position) {0, 0}, (t_position) {1, 7}) == -1, "B8c une foret est inaccessible a pied");
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
    test_batterie();
    test_tour_complet();
    printf("\n%d test(s) reussi(s), %d echec(s)\n", nb_reussis, nb_echecs);
    return nb_echecs == 0 ? 0 : 1;
}
