/*
 * tests.c — Tests automatiques du moteur
 *
 * Le moteur ne dépend pas de l'IHM : on peut le tester sans clavier ni écran,
 * en faisant avancer le temps tick par tick.
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

static int position_vaut(t_position p, int lig, int col)
{
    return p.lig == lig && p.col == col;
}

/* Fait avancer le temps de n ticks */
static void attendre_ticks(t_simulation *sim, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        avancer_temps(sim);
    }
}

static void test_carte(void)
{
    t_simulation sim;

    printf("Carte\n");
    init_simulation(&sim);
    VERIFIER(sim.grille[1][7] == FORET && sim.grille[1][21] == FALAISE && sim.grille[7][15] == EAU,
             "T1  les trois obstacles naturels sont charges");
    VERIFIER(sim.grille[1][1] == CENTRE_PLASTIQUE && sim.grille[7][38] == CENTRE_VERRE
             && sim.grille[11][28] == CENTRE_METAL, "T1b les centres de tri des 3 types sont charges");
    VERIFIER(sim.grille[9][12] == MAISON && position_vaut(sim.technicien.pos, 9, 12)
             && sim.technicien.etat == A_LA_MAISON, "T1c le technicien demarre dans sa maison");
    VERIFIER(position_vaut(sim.drone.pos, 1, 1) && sim.drone.batterie == BATTERIE_MAX,
             "T1d les engins demarrent sur un centre, batterie pleine");
}

static void test_deplacements(void)
{
    t_simulation sim;
    int i;

    printf("Deplacements manuels\n");
    init_simulation(&sim);

    placer(&sim.drone, 0, 5);
    for (i = 0; i < 5; i++) {
        deplacer_engin(&sim, &sim.drone, HAUT);
    }
    VERIFIER(position_vaut(sim.drone.pos, 0, 5), "T2  le drone ne sort pas par le bord haut");

    placer(&sim.drone, 0, 7);   /* (1, 7) : foret */
    VERIFIER(deplacer_engin(&sim, &sim.drone, BAS) == 0 && sim.drone.pos.lig == 0, "T3  pas de traversee de foret");
    placer(&sim.drone, 0, 21);  /* (1, 21) : falaise */
    VERIFIER(deplacer_engin(&sim, &sim.drone, BAS) == 0, "T3b pas de traversee de falaise");
    placer(&sim.drone, 8, 14);  /* (8, 15) : eau */
    VERIFIER(deplacer_engin(&sim, &sim.drone, DROITE) == 0, "T3c pas de traversee de plan d'eau");
    placer(&sim.drone, 9, 11);  /* (9, 12) : maison */
    VERIFIER(deplacer_engin(&sim, &sim.drone, DROITE) == 0, "T3d un engin n'entre pas dans la maison");
    placer(&sim.robot, 4, 0);
    VERIFIER(deplacer_engin(&sim, &sim.robot, DROITE) == 1 && sim.robot.pos.col == 1, "T3e deplacement valide");

    init_simulation(&sim);
    placer(&sim.drone, 4, 4);
    appliquer_commande(&sim, DROITE);
    VERIFIER(position_vaut(sim.drone.pos, 4, 5) && sim.drone.batterie == BATTERIE_MAX - CONSO_DRONE,
             "T4  fleche : une case, le drone use 2 %");
}

static void test_clic(void)
{
    t_simulation sim;

    printf("Deplacement au clic\n");
    init_simulation(&sim);
    VERIFIER(definir_destination(&sim, 1, 7) == 0, "C1  clic sur une foret : refuse");
    VERIFIER(definir_destination(&sim, 9, 12) == 0, "C1b clic sur la maison : refuse");
    VERIFIER(definir_destination(&sim, 0, 10) == 1 && sim.drone.a_destination, "C2  clic valide : trajet lance");

    attendre_ticks(&sim, 1);
    VERIFIER(!position_vaut(sim.drone.pos, 1, 1), "C3  l'engin avance tout seul");
    attendre_ticks(&sim, 60);
    VERIFIER(position_vaut(sim.drone.pos, 0, 10) && !sim.drone.a_destination, "C4  l'engin arrive a destination");
    VERIFIER(sim.drone.batterie == BATTERIE_MAX - 10 * CONSO_DRONE, "C4b plus court chemin : 10 cases, 20 %");

    init_simulation(&sim);
    placer(&sim.drone, 0, 5);
    definir_destination(&sim, 4, 8);   /* il faut contourner la foret */
    attendre_ticks(&sim, 100);
    VERIFIER(position_vaut(sim.drone.pos, 4, 8), "C5  le trajet contourne les obstacles");

    init_simulation(&sim);
    definir_destination(&sim, 0, 30);
    attendre_ticks(&sim, 4);
    appliquer_commande(&sim, BAS);
    VERIFIER(!sim.drone.a_destination, "C6  une fleche annule le trajet automatique");

    init_simulation(&sim);
    ajouter_dechet(&sim, 0, 6, VERRE);
    sim.dechets[0].detecte = 1;
    appliquer_commande(&sim, CHANGER_ENGIN);
    definir_destination(&sim, 0, 9);
    attendre_ticks(&sim, 60);
    VERIFIER(sim.robot.charge[VERRE] == 1, "C7  le robot ramasse un dechet sur son trajet");
}

static void test_detection(void)
{
    t_simulation sim;

    printf("Detection (F6)\n");

    init_simulation(&sim);
    ajouter_dechet(&sim, 5, 10, PLASTIQUE);
    placer(&sim.drone, 4, 9);
    VERIFIER(detecter_dechets(&sim) == 1 && dechet_visible_en(&sim, 5, 10), "D1  dechet detecte en diagonale");

    init_simulation(&sim);
    ajouter_dechet(&sim, 5, 10, PLASTIQUE);
    placer(&sim.drone, 3, 10);
    VERIFIER(detecter_dechets(&sim) == 0 && !dechet_visible_en(&sim, 5, 10), "D2  dechet a 2 cases : pas detecte");

    init_simulation(&sim);
    ajouter_dechet(&sim, 0, 0, METAL);
    placer(&sim.drone, 0, 1);
    VERIFIER(detecter_dechets(&sim) == 1 && sim.stats.nb_detectes[METAL] == 1,
             "D3  dechet dans un coin detecte et compte dans son type");
    VERIFIER(detecter_dechets(&sim) == 0, "D3b un dechet n'est compte qu'une fois");
}

static void test_tri(void)
{
    t_simulation sim;

    printf("Ramassage et tri (E2)\n");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10, VERRE);
    sim.dechets[0].detecte = 1;
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 1 && sim.robot.charge[VERRE] == 1, "R1  le robot ramasse un dechet detecte");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10, VERRE);
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 0, "R2  dechet non detecte : rien ne se passe");

    init_simulation(&sim);
    ajouter_dechet(&sim, 4, 10, METAL);
    sim.dechets[0].detecte = 1;
    sim.robot.charge[PLASTIQUE] = 2;
    sim.robot.charge[VERRE] = 1;
    placer(&sim.robot, 4, 10);
    VERIFIER(ramasser_dechet(&sim) == 0 && indice_dechet_en(&sim, 4, 10) != -1,
             "R3  robot plein (3 dechets, tous types) : il ne prend plus rien");

    init_simulation(&sim);
    sim.robot.charge[PLASTIQUE] = 1;
    sim.robot.charge[VERRE] = 2;
    placer(&sim.robot, 1, 1);   /* centre plastique */
    VERIFIER(deposer_dechets(&sim) == 1 && sim.robot.charge[PLASTIQUE] == 0 && sim.robot.charge[VERRE] == 2,
             "R4  un centre plastique ne prend que le plastique");
    placer(&sim.robot, 7, 38);  /* centre verre */
    VERIFIER(deposer_dechets(&sim) == 2 && charge_totale(&sim.robot) == 0 && sim.stats.nb_collectes[VERRE] == 2,
             "R4b le verre est trie au centre verre");
    sim.robot.charge[PLASTIQUE] = 1;
    placer(&sim.robot, 11, 28); /* centre metal */
    VERIFIER(deposer_dechets(&sim) == 0 && sim.robot.charge[PLASTIQUE] == 1,
             "R4c rien a deposer dans un centre d'un autre type");
}

static void test_apparition(void)
{
    t_simulation sim;
    int lig, col, ajoutes = 0;

    printf("Apparition des dechets (F5)\n");
    init_simulation(&sim);

    VERIFIER(ajouter_dechet(&sim, 1, 7, PLASTIQUE) == 0, "A1  pas de dechet dans une foret");
    VERIFIER(ajouter_dechet(&sim, 2, 3, PLASTIQUE) == 0 && ajouter_dechet(&sim, 9, 12, PLASTIQUE) == 0
             && ajouter_dechet(&sim, 7, 38, VERRE) == 0, "A1b ni sur une station, ni sur la maison, ni sur un centre");
    VERIFIER(ajouter_dechet(&sim, -1, 3, METAL) == 0 && ajouter_dechet(&sim, 3, NB_COLONNES, METAL) == 0,
             "A1c pas de dechet hors grille");
    VERIFIER(ajouter_dechet(&sim, 4, 4, (t_type_dechet) 7) == 0, "A1d type de dechet invalide refuse");

    for (lig = 0; lig < NB_LIGNES; lig++) {
        for (col = 0; col < NB_COLONNES; col++) {
            ajoutes += ajouter_dechet(&sim, lig, col, PLASTIQUE);
        }
    }
    VERIFIER(ajoutes == MAX_DECHETS, "A2  jamais plus de MAX_DECHETS dechets");
    sim.dechets[0].actif = 0;
    VERIFIER(ajouter_dechet(&sim, 14, 29, VERRE) == 1 && sim.nb_dechets == MAX_DECHETS,
             "A3  la place d'un dechet ramasse est reutilisee");

    init_simulation(&sim);
    attendre_ticks(&sim, 3000);   /* 5 minutes */
    VERIFIER(sim.stats.nb_apparus > 10, "A4  des dechets apparaissent avec le temps");
}

static void test_recharge(void)
{
    t_simulation sim;

    printf("Batteries et recharge de 5 secondes (E3)\n");
    init_simulation(&sim);
    sim.engin_actif = ROBOT;
    placer(&sim.robot, 4, 4);
    sim.robot.charge[METAL] = 3;
    appliquer_commande(&sim, DROITE);
    VERIFIER(sim.robot.batterie == BATTERIE_MAX - 4, "B1  le robot plein use 4 % par case");

    init_simulation(&sim);
    placer(&sim.drone, 2, 4);
    sim.drone.batterie = 10;
    appliquer_commande(&sim, GAUCHE);   /* arrive sur la station (2, 3) */
    VERIFIER(sim.drone.etat == EN_RECHARGE && sim.drone.batterie == 8, "B2  arrivee sur une station : recharge");
    appliquer_commande(&sim, DROITE);
    VERIFIER(position_vaut(sim.drone.pos, 2, 3), "B3  pendant la recharge, l'engin ne bouge pas");
    attendre_ticks(&sim, DUREE_RECHARGE / 2);
    VERIFIER(sim.drone.batterie > 40 && sim.drone.batterie < 70, "B4  la batterie remonte peu a peu");
    attendre_ticks(&sim, DUREE_RECHARGE / 2 - 1);
    VERIFIER(sim.drone.etat == EN_RECHARGE, "B5  la recharge n'est pas finie avant 5 secondes");
    attendre_ticks(&sim, 1);
    VERIFIER(sim.drone.etat == OPERATIONNEL && sim.drone.batterie == BATTERIE_MAX,
             "B5b au bout de 5 secondes : 100 % et de nouveau pilotable");
}

static void test_technicien(void)
{
    t_simulation sim;
    int t, vu_va_chercher = 0, vu_porte = 0, chemin_valide = 1, porte_ensemble = 1, deux_portes = 0;

    printf("Panne et technicien\n");
    init_simulation(&sim);
    placer(&sim.drone, 4, 4);
    sim.drone.batterie = 2;
    appliquer_commande(&sim, DROITE);   /* (4, 5), batterie a 0 */
    VERIFIER(sim.drone.etat == EN_PANNE && position_vaut(sim.drone.pos, 4, 5) && sim.stats.nb_pannes == 1,
             "P1  batterie vide : le drone s'arrete sur place (pas de teleportation)");
    VERIFIER(definir_destination(&sim, 0, 0) == 0, "P1b un engin en panne ne se pilote plus");

    for (t = 0; t < 3000 && !(sim.technicien.etat == A_LA_MAISON && sim.drone.etat == OPERATIONNEL); t++) {
        avancer_temps(&sim);
        if (sim.technicien.etat == VA_CHERCHER) {
            vu_va_chercher = 1;
        }
        if (sim.drone.etat == TRANSPORTE) {
            vu_porte = 1;
            if (!position_vaut(sim.drone.pos, sim.technicien.pos.lig, sim.technicien.pos.col)) {
                porte_ensemble = 0;
            }
        }
        if (!case_praticable_a_pied(&sim, sim.technicien.pos.lig, sim.technicien.pos.col)) {
            chemin_valide = 0;
        }
    }
    VERIFIER(vu_va_chercher, "P2  le technicien sort de sa maison et va chercher le drone");
    VERIFIER(vu_porte && porte_ensemble, "P3  il porte le drone (ils avancent ensemble)");
    VERIFIER(chemin_valide, "P4  le technicien ne marche que sur des cases praticables");
    VERIFIER(position_vaut(sim.drone.pos, 2, 3), "P5  le drone est depose a la station la plus proche");
    VERIFIER(sim.drone.etat == OPERATIONNEL && sim.drone.batterie == BATTERIE_MAX, "P6  puis il est recharge");
    VERIFIER(sim.technicien.etat == A_LA_MAISON && position_vaut(sim.technicien.pos, 9, 12),
             "P7  le technicien est rentre chez lui");

    /* Deux pannes en meme temps : un seul technicien les traite l'une apres l'autre */
    init_simulation(&sim);
    placer(&sim.drone, 0, 20);
    placer(&sim.robot, 18, 10);
    sim.robot.charge[VERRE] = 2;
    declencher_panne(&sim, &sim.drone);
    declencher_panne(&sim, &sim.robot);
    for (t = 0; t < 4000; t++) {
        avancer_temps(&sim);
        if (sim.drone.etat == TRANSPORTE && sim.robot.etat == TRANSPORTE) {
            deux_portes = 1;
        }
    }
    VERIFIER(!deux_portes, "P8  un seul technicien : il ne porte qu'un engin a la fois");
    VERIFIER(sim.drone.etat == OPERATIONNEL && sim.robot.etat == OPERATIONNEL
             && sim.technicien.etat == A_LA_MAISON, "P9  les deux engins sont sauves, le technicien est rentre");
    VERIFIER(sim.robot.charge[VERRE] == 2, "P10 le robot garde ses dechets pendant la panne");
}

static void test_commandes(void)
{
    t_simulation sim;

    printf("Commandes (F7, F12)\n");
    init_simulation(&sim);
    appliquer_commande(&sim, CHANGER_ENGIN);
    VERIFIER(sim.engin_actif == ROBOT, "K1  Tab : on pilote le robot");
    appliquer_commande(&sim, AUCUNE);
    VERIFIER(sim.engin_actif == ROBOT && sim.en_cours, "K2  aucune commande : rien ne change");
    appliquer_commande(&sim, QUITTER);
    VERIFIER(sim.en_cours == 0, "K3  Echap : la simulation s'arrete");
    attendre_ticks(&sim, 10);
    VERIFIER(sim.stats.nb_ticks == 0, "K4  plus rien n'avance apres l'arret");
}

int main(void)
{
    printf("===== Tests du moteur EcoBots =====\n\n");
    test_carte();
    test_deplacements();
    test_clic();
    test_detection();
    test_tri();
    test_apparition();
    test_recharge();
    test_technicien();
    test_commandes();
    printf("\n%d test(s) reussi(s), %d echec(s)\n", nb_reussis, nb_echecs);
    return nb_echecs == 0 ? 0 : 1;
}
