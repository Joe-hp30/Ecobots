/*
 * main_raylib.c — Programme principal EcoBots, mode graphique (raylib)
 *
 * Même moteur que la version console (simulation.c) : seuls l'affichage et la saisie changent.
 * La fenêtre est redessinée 60 fois par seconde ; la simulation, elle, avance de
 * TICKS_PAR_SECONDE ticks par seconde, comme en console.
 *
 * Fonctions raylib utilisées pour la saisie :
 *   IsKeyPressed, IsKeyPressedRepeat (flèches, Tab, Entrée, Échap),
 *   GetCharPressed (lettres Z Q S D, quel que soit le clavier),
 *   IsMouseButtonPressed, GetMouseX, GetMouseY (clic gauche).
 */

#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "simulation.h"
#include "affichage_raylib.h"

typedef enum {
    ECRAN_MENU,
    ECRAN_JEU,
    ECRAN_BILAN
} t_ecran;

/* Touche maintenue : répétée automatiquement */
static int touche(int code)
{
    return IsKeyPressed(code) || IsKeyPressedRepeat(code);
}

/* Lit clavier et souris pendant le jeu et les transmet au moteur */
static void traiter_saisies(t_simulation *sim)
{
    t_position cible;
    int c;

    if (touche(KEY_UP))    appliquer_commande(sim, HAUT);
    if (touche(KEY_DOWN))  appliquer_commande(sim, BAS);
    if (touche(KEY_LEFT))  appliquer_commande(sim, GAUCHE);
    if (touche(KEY_RIGHT)) appliquer_commande(sim, DROITE);
    if (IsKeyPressed(KEY_TAB))    appliquer_commande(sim, CHANGER_ENGIN);
    if (IsKeyPressed(KEY_ESCAPE)) appliquer_commande(sim, QUITTER);

    /* Lettres tapées (clavier AZERTY ou QWERTY) */
    while ((c = GetCharPressed()) != 0) {
        switch (c) {
            case 'z': case 'Z': appliquer_commande(sim, HAUT); break;
            case 's': case 'S': appliquer_commande(sim, BAS); break;
            case 'q': case 'Q': appliquer_commande(sim, GAUCHE); break;
            case 'd': case 'D': appliquer_commande(sim, DROITE); break;
            default: break;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && souris_vers_case(GetMouseX(), GetMouseY(), &cible)) {
        definir_destination(sim, cible.lig, cible.col);
    }
}

int main(void)
{
    t_simulation sim;
    t_ecran ecran = ECRAN_MENU;
    float temps_accumule = 0.0f;
    const float duree_tick = 1.0f / TICKS_PAR_SECONDE;
    int fin = 0;

    srand((unsigned int) time(NULL));
    init_simulation(&sim);

    InitWindow(LARGEUR_FENETRE, HAUTEUR_FENETRE, "EcoBots");
    SetExitKey(KEY_NULL);   /* Échap ne ferme pas la fenêtre : c'est nous qui le gérons */
    SetTargetFPS(60);

    while (!fin && !WindowShouldClose()) {
        /* ----- Mise à jour ----- */
        switch (ecran) {
            case ECRAN_MENU:
                if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    init_simulation(&sim);
                    temps_accumule = 0.0f;
                    ecran = ECRAN_JEU;
                } else if (IsKeyPressed(KEY_ESCAPE)) {
                    fin = 1;
                }
                break;

            case ECRAN_JEU:
                traiter_saisies(&sim);
                temps_accumule += GetFrameTime();
                while (temps_accumule >= duree_tick && sim.en_cours) {
                    avancer_temps(&sim);   /* 10 ticks par seconde, quel que soit le nombre d'images */
                    temps_accumule -= duree_tick;
                }
                if (!sim.en_cours) {
                    ecran = ECRAN_BILAN;
                }
                break;

            case ECRAN_BILAN:
                if (IsKeyPressed(KEY_ENTER)) {
                    ecran = ECRAN_MENU;
                } else if (IsKeyPressed(KEY_ESCAPE)) {
                    fin = 1;
                }
                break;
        }

        /* ----- Dessin ----- */
        BeginDrawing();
        switch (ecran) {
            case ECRAN_MENU:  dessiner_menu(); break;
            case ECRAN_JEU:   dessiner_simulation(&sim, GetMouseX(), GetMouseY()); break;
            case ECRAN_BILAN: dessiner_bilan(&sim); break;
        }
        EndDrawing();
    }

    CloseWindow();
    return EXIT_SUCCESS;
}
