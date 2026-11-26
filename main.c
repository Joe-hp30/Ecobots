/*
 * main.c — Programme principal EcoBots
 *
 * Relie les trois modules : saisie (clavier, souris) -> simulation (moteur) -> affichage (écran).
 * C'est le seul fichier qui connaît à la fois l'IHM et le moteur.
 */

#include <stdlib.h>
#include <time.h>
#include "simulation.h"
#include "affichage.h"
#include "saisie.h"

/* Traite toutes les touches et tous les clics arrivés depuis le dernier tick */
static void traiter_saisies(t_simulation *sim)
{
    t_commande cmd;
    t_position case_cliquee;
    int x, y;

    while ((cmd = lire_commande(&x, &y)) != AUCUNE && sim->en_cours) {
        if (cmd == CLIC) {
            if (ecran_vers_case(x, y, &case_cliquee)) {
                definir_destination(sim, case_cliquee.lig, case_cliquee.col);
            }
        } else {
            appliquer_commande(sim, cmd);
        }
    }
}

/* Joue une simulation en temps réel, jusqu'à ce que l'utilisateur appuie sur Échap */
static void lancer_simulation(void)
{
    t_simulation sim;

    init_simulation(&sim);
    preparer_ecran_simulation();
    while (sim.en_cours) {
        traiter_saisies(&sim);                 /* 1. lire clavier et souris */
        avancer_temps(&sim);                   /* 2. faire avancer le monde d'un tick */
        afficher_simulation(&sim);             /* 3. redessiner */
        attendre_ms(1000 / TICKS_PAR_SECONDE); /* 4. attendre le tick suivant */
    }
    afficher_bilan(&sim);
    attendre_touche();
}

int main(void)
{
    int choix;

    srand((unsigned int) time(NULL));   /* une seule fois, au tout début */
    affichage_initialiser();
    saisie_initialiser();

    do {
        afficher_menu();
        choix = lire_choix_menu(1, 3);

        if (choix == 1) {
            lancer_simulation();
        } else if (choix == 2) {
            afficher_regles();
            attendre_touche();
        }
    } while (choix != 3);

    afficher_au_revoir();
    saisie_terminer();   /* rend au terminal son fonctionnement normal */
    return EXIT_SUCCESS;
}
