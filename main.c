/*
 * main.c — Programme principal EcoBots
 *
 * Relie les trois modules : saisie (clavier) -> simulation (moteur) -> affichage (écran).
 * C'est le seul fichier qui connaît à la fois l'IHM et le moteur.
 */

#include <stdlib.h>
#include <time.h>
#include "simulation.h"
#include "affichage.h"
#include "saisie.h"

/* Joue une simulation complète, jusqu'à ce que l'utilisateur appuie sur Échap */
static void lancer_simulation(void)
{
    t_simulation sim;

    init_simulation(&sim);
    while (sim.en_cours) {
        afficher_simulation(&sim);                 /* 1. afficher            */
        appliquer_commande(&sim, lire_commande()); /* 2. lire + 3. appliquer */
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
