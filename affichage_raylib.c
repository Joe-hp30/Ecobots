/*
 * affichage_raylib.c — Affichage graphique avec raylib (partie IHM, mode graphique)
 *
 * Tout est dessiné avec des formes simples de raylib (rectangles, cercles, triangles, texte) :
 * pas d'image externe à charger.
 *
 * Fonctions raylib utilisées :
 *   ClearBackground, DrawRectangle, DrawRectangleRec, DrawRectangleLinesEx, DrawRectangleRounded,
 *   DrawCircle, DrawCircleLines, DrawTriangle, DrawLineEx, DrawText, MeasureText, Fade.
 * Elles sont toutes appelées entre BeginDrawing() et EndDrawing(), dans main_raylib.c.
 */

#include <stdio.h>
#include "raylib.h"
#include "affichage_raylib.h"

/* ---------- Couleurs ---------- */

static const Color C_HERBE      = {196, 224, 160, 255};
static const Color C_HERBE2     = {186, 216, 150, 255};
static const Color C_FORET      = {46, 110, 50, 255};
static const Color C_ARBRE      = {30, 80, 36, 255};
static const Color C_FALAISE    = {128, 120, 112, 255};
static const Color C_ROCHE      = {96, 90, 84, 255};
static const Color C_EAU        = {70, 140, 210, 255};
static const Color C_VAGUE      = {140, 190, 240, 255};
static const Color C_STATION    = {250, 210, 60, 255};
static const Color C_MAISON     = {205, 120, 80, 255};
static const Color C_TOIT       = {140, 50, 40, 255};
static const Color C_DRONE      = {40, 90, 200, 255};
static const Color C_ROBOT      = {235, 130, 30, 255};
static const Color C_TECH       = {60, 60, 70, 255};
static const Color C_PANNEAU    = {245, 245, 240, 255};
static const Color C_TEXTE      = {40, 40, 45, 255};
static const Color C_GRIS       = {120, 120, 125, 255};

/* Plastique, verre, métal */
static const Color C_TYPE[NB_TYPES_DECHETS] = {{240, 190, 0, 255}, {30, 170, 150, 255}, {150, 90, 190, 255}};
static const char *LETTRE_CENTRE[NB_TYPES_DECHETS] = {"P", "V", "M"};
static const char *NOM_TYPE[NB_TYPES_DECHETS] = {"Plastique", "Verre", "Metal"};

/* ---------- Outils ---------- */

/* Coin haut-gauche d'une case à l'écran */
static int ecran_x(int col) { return col * TAILLE_CASE; }
static int ecran_y(int lig) { return HAUT_BANDEAU + lig * TAILLE_CASE; }

/* Centre d'une case à l'écran */
static Vector2 centre_case(t_position p)
{
    Vector2 v;
    v.x = (float) (ecran_x(p.col) + TAILLE_CASE / 2);
    v.y = (float) (ecran_y(p.lig) + TAILLE_CASE / 2);
    return v;
}

/* Texte centré horizontalement autour de x */
static void texte_centre(const char *texte, int x, int y, int taille, Color couleur)
{
    DrawText(texte, x - MeasureText(texte, taille) / 2, y, taille, couleur);
}

int souris_vers_case(int x, int y, t_position *p)
{
    int lig, col;

    if (p == NULL || x < 0 || y < HAUT_BANDEAU) {
        return 0;
    }
    lig = (y - HAUT_BANDEAU) / TAILLE_CASE;
    col = x / TAILLE_CASE;
    if (!est_dans_grille(lig, col)) {
        return 0;
    }
    p->lig = lig;
    p->col = col;
    return 1;
}

/* ---------- Terrain ---------- */

static void dessiner_terrain(t_case terrain, int lig, int col)
{
    int x = ecran_x(col), y = ecran_y(lig), t = TAILLE_CASE;
    float cx = (float) (x + t / 2), cy = (float) (y + t / 2);

    /* Herbe en damier très léger, sous tout le reste */
    DrawRectangle(x, y, t, t, ((lig + col) % 2 == 0) ? C_HERBE : C_HERBE2);

    switch (terrain) {
        case FORET:
            DrawRectangle(x, y, t, t, C_FORET);
            DrawCircle((int) cx - 4, (int) cy + 2, 6, C_ARBRE);
            DrawCircle((int) cx + 5, (int) cy - 3, 6, C_ARBRE);
            break;
        case FALAISE:
            DrawRectangle(x, y, t, t, C_FALAISE);
            DrawTriangle((Vector2) {cx, (float) y + 3}, (Vector2) {(float) x + 3, (float) (y + t - 3)},
                         (Vector2) {(float) (x + t - 3), (float) (y + t - 3)}, C_ROCHE);
            break;
        case EAU:
            DrawRectangle(x, y, t, t, C_EAU);
            DrawLineEx((Vector2) {(float) x + 4, cy - 3}, (Vector2) {(float) x + 11, cy - 3}, 2, C_VAGUE);
            DrawLineEx((Vector2) {(float) x + 12, cy + 4}, (Vector2) {(float) x + 19, cy + 4}, 2, C_VAGUE);
            break;
        case STATION:
            DrawRectangleRounded((Rectangle) {(float) x + 2, (float) y + 2, (float) t - 4, (float) t - 4},
                                 0.3f, 4, C_STATION);
            /* éclair */
            DrawTriangle((Vector2) {cx + 2, (float) y + 4}, (Vector2) {cx - 5, cy + 1}, (Vector2) {cx + 1, cy + 1}, C_TEXTE);
            DrawTriangle((Vector2) {cx - 1, cy - 1}, (Vector2) {cx - 2, (float) (y + t - 4)}, (Vector2) {cx + 5, cy - 1}, C_TEXTE);
            break;
        case MAISON:
            DrawRectangle(x + 4, (int) cy - 1, t - 8, t / 2 - 2, C_MAISON);
            DrawTriangle((Vector2) {cx, (float) y + 2}, (Vector2) {(float) x + 2, cy}, (Vector2) {(float) (x + t - 2), cy}, C_TOIT);
            DrawRectangle((int) cx - 2, (int) cy + 4, 5, 7, C_TOIT);
            break;
        case CENTRE_PLASTIQUE:
        case CENTRE_VERRE:
        case CENTRE_METAL:
            DrawRectangleRounded((Rectangle) {(float) x + 1, (float) y + 1, (float) t - 2, (float) t - 2},
                                 0.25f, 4, C_TYPE[type_du_centre(terrain)]);
            texte_centre(LETTRE_CENTRE[type_du_centre(terrain)], (int) cx, y + 5, 16, WHITE);
            break;
        default:
            break;
    }
}

/* ---------- Objets ---------- */

static void dessiner_dechet(const t_dechet *d)
{
    Vector2 c = centre_case(d->pos);

    DrawCircle((int) c.x, (int) c.y, 7, C_TYPE[d->type]);
    DrawCircleLines((int) c.x, (int) c.y, 7, C_TEXTE);
}

/* Contour qui signale l'état d'un engin : rouge = panne, vert = recharge */
static void dessiner_etat(const t_engin *engin, Vector2 c)
{
    if (engin->etat == EN_PANNE) {
        DrawCircleLines((int) c.x, (int) c.y, 12, RED);
        DrawCircleLines((int) c.x, (int) c.y, 11, RED);
    } else if (engin->etat == EN_RECHARGE) {
        float part = (float) (DUREE_RECHARGE - engin->temps_recharge) / DUREE_RECHARGE;
        DrawRectangle((int) c.x - 10, (int) c.y + 9, 20, 3, DARKGRAY);
        DrawRectangle((int) c.x - 10, (int) c.y + 9, (int) (20 * part), 3, GREEN);
    }
}

static void dessiner_drone(const t_engin *drone, int pilote)
{
    Vector2 c = centre_case(drone->pos);
    Color couleur = (drone->etat == EN_PANNE || drone->etat == TRANSPORTE) ? GRAY : C_DRONE;

    if (pilote) {
        DrawCircle((int) c.x, (int) c.y, 12, Fade(WHITE, 0.6f));
    }
    DrawLineEx((Vector2) {c.x - 7, c.y - 7}, (Vector2) {c.x + 7, c.y + 7}, 2, couleur);
    DrawLineEx((Vector2) {c.x - 7, c.y + 7}, (Vector2) {c.x + 7, c.y - 7}, 2, couleur);
    DrawCircle((int) c.x - 7, (int) c.y - 7, 3, couleur);
    DrawCircle((int) c.x + 7, (int) c.y - 7, 3, couleur);
    DrawCircle((int) c.x - 7, (int) c.y + 7, 3, couleur);
    DrawCircle((int) c.x + 7, (int) c.y + 7, 3, couleur);
    DrawCircle((int) c.x, (int) c.y, 4, couleur);
    dessiner_etat(drone, c);
}

static void dessiner_robot(const t_engin *robot, int pilote)
{
    Vector2 c = centre_case(robot->pos);
    Color couleur = (robot->etat == EN_PANNE || robot->etat == TRANSPORTE) ? GRAY : C_ROBOT;

    if (pilote) {
        DrawCircle((int) c.x, (int) c.y, 12, Fade(WHITE, 0.6f));
    }
    DrawRectangleRounded((Rectangle) {c.x - 9, c.y - 7, 18, 11}, 0.3f, 4, couleur);
    DrawCircle((int) c.x - 5, (int) c.y + 6, 3, C_TEXTE);
    DrawCircle((int) c.x + 5, (int) c.y + 6, 3, C_TEXTE);
    if (charge_totale(robot) > 0) {   /* benne pleine */
        DrawRectangle((int) c.x - 6, (int) c.y - 10, 12, 4, DARKGRAY);
    }
    dessiner_etat(robot, c);
}

static void dessiner_technicien(const t_technicien *tech)
{
    Vector2 c = centre_case(tech->pos);

    if (tech->etat == A_LA_MAISON) {
        return;   /* il est dans sa maison */
    }
    DrawCircle((int) c.x, (int) c.y - 6, 4, C_TECH);
    DrawLineEx((Vector2) {c.x, c.y - 2}, (Vector2) {c.x, c.y + 5}, 2, C_TECH);
    DrawLineEx((Vector2) {c.x, c.y + 5}, (Vector2) {c.x - 4, c.y + 11}, 2, C_TECH);
    DrawLineEx((Vector2) {c.x, c.y + 5}, (Vector2) {c.x + 4, c.y + 11}, 2, C_TECH);
    DrawLineEx((Vector2) {c.x - 5, c.y + 1}, (Vector2) {c.x + 5, c.y + 1}, 2, C_TECH);
}

/* Trajet prévu de l'engin piloté : petits points jusqu'à la destination */
static void dessiner_trajet(const t_simulation *sim, const t_engin *engin)
{
    t_position p = engin->pos, suivant;
    Vector2 c;
    int pas;

    if (!engin->a_destination) {
        return;
    }
    for (pas = 0; pas < NB_LIGNES * NB_COLONNES; pas++) {
        suivant = prochain_pas(sim, p, engin->destination, 0);
        if (suivant.lig == p.lig && suivant.col == p.col) {
            break;
        }
        p = suivant;
        c = centre_case(p);
        DrawCircle((int) c.x, (int) c.y, 2, Fade(C_TEXTE, 0.5f));
    }
    DrawRectangleLinesEx((Rectangle) {(float) ecran_x(engin->destination.col), (float) ecran_y(engin->destination.lig),
                                      TAILLE_CASE, TAILLE_CASE}, 2, C_TEXTE);
}

/* ---------- Panneau d'infos ---------- */

static void barre_batterie(int x, int y, const t_engin *engin)
{
    char texte[48];
    Color couleur = engin->batterie <= SEUIL_BATTERIE_FAIBLE ? RED : (Color) {60, 170, 70, 255};

    DrawRectangle(x, y, 150, 14, LIGHTGRAY);
    DrawRectangle(x, y, 150 * engin->batterie / BATTERIE_MAX, 14, couleur);
    DrawRectangleLines(x, y, 150, 14, C_GRIS);

    switch (engin->etat) {
        case EN_RECHARGE:
            snprintf(texte, sizeof(texte), "%d %% recharge %d s", engin->batterie,
                     (engin->temps_recharge + TICKS_PAR_SECONDE - 1) / TICKS_PAR_SECONDE);
            break;
        case EN_PANNE:   snprintf(texte, sizeof(texte), "EN PANNE"); break;
        case TRANSPORTE: snprintf(texte, sizeof(texte), "porte"); break;
        default:         snprintf(texte, sizeof(texte), "%d %%", engin->batterie); break;
    }
    DrawText(texte, x + 158, y, 14, engin->etat == EN_PANNE || engin->etat == TRANSPORTE ? RED : C_TEXTE);
}

static void dessiner_panneau(const t_simulation *sim)
{
    const t_technicien *tech = &sim->technicien;
    const char *cible = (tech->cible == DRONE) ? "drone" : "robot";
    char texte[96];
    int x = LARGEUR_GRILLE + 16, y = HAUT_BANDEAU + 10, t;

    DrawRectangle(LARGEUR_GRILLE, HAUT_BANDEAU, LARGEUR_PANNEAU, HAUTEUR_FENETRE - HAUT_BANDEAU, C_PANNEAU);

    DrawText("Engins", x, y, 18, C_TEXTE);
    DrawText(sim->engin_actif == DRONE ? "> Drone" : "  Drone", x, y + 28, 14, C_DRONE);
    barre_batterie(x + 70, y + 28, &sim->drone);
    DrawText(sim->engin_actif == ROBOT ? "> Robot" : "  Robot", x, y + 50, 14, C_ROBOT);
    barre_batterie(x + 70, y + 50, &sim->robot);

    snprintf(texte, sizeof(texte), "Robot : %d / %d dechets", charge_totale(&sim->robot), CAPACITE_ROBOT);
    DrawText(texte, x, y + 76, 14, C_TEXTE);
    for (t = 0; t < NB_TYPES_DECHETS; t++) {
        int bx = x + 175 + t * 42;
        DrawCircle(bx, y + 83, 6, C_TYPE[t]);
        snprintf(texte, sizeof(texte), "%d", sim->robot.charge[t]);
        DrawText(texte, bx + 10, y + 76, 14, C_TEXTE);
    }

    DrawText("Technicien", x, y + 112, 18, C_TEXTE);
    switch (tech->etat) {
        case A_LA_MAISON: snprintf(texte, sizeof(texte), "A la maison"); break;
        case VA_CHERCHER: snprintf(texte, sizeof(texte), "Va chercher le %s", cible); break;
        case RAMENE:      snprintf(texte, sizeof(texte), "Porte le %s a la station", cible); break;
        default:          snprintf(texte, sizeof(texte), "Rentre chez lui"); break;
    }
    DrawText(texte, x, y + 138, 14, tech->etat == A_LA_MAISON ? C_GRIS : C_TEXTE);

    DrawText("Dechets", x, y + 172, 18, C_TEXTE);
    DrawText("detectes   tries   au sol", x + 100, y + 198, 12, C_GRIS);
    for (t = 0; t < NB_TYPES_DECHETS; t++) {
        int ly = y + 218 + t * 22;
        DrawCircle(x + 6, ly + 6, 6, C_TYPE[t]);
        DrawText(NOM_TYPE[t], x + 18, ly, 14, C_TEXTE);
        snprintf(texte, sizeof(texte), "%d", sim->stats.nb_detectes[t]);
        DrawText(texte, x + 120, ly, 14, C_TEXTE);
        snprintf(texte, sizeof(texte), "%d", sim->stats.nb_collectes[t]);
        DrawText(texte, x + 180, ly, 14, C_TEXTE);
        snprintf(texte, sizeof(texte), "%d", nb_dechets_en_attente_type(sim, (t_type_dechet) t));
        DrawText(texte, x + 232, ly, 14, C_TEXTE);
    }
    snprintf(texte, sizeof(texte), "Pannes : %d    Recharges : %d", sim->stats.nb_pannes, sim->stats.nb_recharges);
    DrawText(texte, x, y + 290, 14, C_TEXTE);

    DrawText("Commandes", x, y + 326, 18, C_TEXTE);
    DrawText("Clic gauche : y envoyer l'engin", x, y + 352, 14, C_TEXTE);
    DrawText("Fleches / ZQSD : une case", x, y + 372, 14, C_TEXTE);
    DrawText("Tab : changer d'engin", x, y + 392, 14, C_TEXTE);
    DrawText("Echap : terminer", x, y + 412, 14, C_TEXTE);
}

/* ---------- Écrans ---------- */

void dessiner_simulation(const t_simulation *sim, int souris_x, int souris_y)
{
    const t_engin *pilote = (sim->engin_actif == DRONE) ? &sim->drone : &sim->robot;
    t_position survol;
    char texte[64];
    int lig, col, i, secondes = sim->stats.nb_ticks / TICKS_PAR_SECONDE;

    ClearBackground(RAYWHITE);

    for (lig = 0; lig < NB_LIGNES; lig++) {
        for (col = 0; col < NB_COLONNES; col++) {
            dessiner_terrain(sim->grille[lig][col], lig, col);
        }
    }
    for (i = 0; i < sim->nb_dechets; i++) {
        if (sim->dechets[i].actif && sim->dechets[i].detecte) {
            dessiner_dechet(&sim->dechets[i]);
        }
    }

    /* Case survolée : cadre blanc, ou rouge si l'engin ne peut pas y aller */
    if (souris_vers_case(souris_x, souris_y, &survol)) {
        DrawRectangleLinesEx((Rectangle) {(float) ecran_x(survol.col), (float) ecran_y(survol.lig),
                                          TAILLE_CASE, TAILLE_CASE}, 2,
                             case_franchissable(sim, survol.lig, survol.col) ? WHITE : RED);
    }

    dessiner_trajet(sim, pilote);
    dessiner_robot(&sim->robot, sim->engin_actif == ROBOT);
    dessiner_drone(&sim->drone, sim->engin_actif == DRONE);
    dessiner_technicien(&sim->technicien);

    /* Bandeau du haut */
    DrawRectangle(0, 0, LARGEUR_FENETRE, HAUT_BANDEAU, (Color) {35, 70, 45, 255});
    DrawText("EcoBots - Protection d'une reserve naturelle", 14, 12, 20, RAYWHITE);
    snprintf(texte, sizeof(texte), "%02d:%02d", secondes / 60, secondes % 60);
    DrawText(texte, LARGEUR_FENETRE - 80, 12, 20, RAYWHITE);

    dessiner_panneau(sim);

    /* Message du moteur, en bas */
    DrawRectangle(0, HAUT_BANDEAU + HAUTEUR_GRILLE, LARGEUR_GRILLE, 40, (Color) {230, 230, 225, 255});
    DrawText(sim->message, 12, HAUT_BANDEAU + HAUTEUR_GRILLE + 12, 16, C_TEXTE);
}

void dessiner_menu(void)
{
    int cx = LARGEUR_FENETRE / 2;

    ClearBackground((Color) {35, 70, 45, 255});
    texte_centre("ECOBOTS", cx, 120, 60, RAYWHITE);
    texte_centre("Protection d'une reserve naturelle", cx, 195, 24, C_HERBE);
    texte_centre("Le drone detecte les dechets, le robot les trie (P plastique, V verre, M metal).",
                 cx, 270, 18, RAYWHITE);
    texte_centre("Batterie vide : le technicien vient chercher l'engin a pied.", cx, 298, 18, RAYWHITE);
    texte_centre("Recharge sur une station : 5 secondes.", cx, 326, 18, RAYWHITE);
    texte_centre("Entree ou clic : commencer      Echap : quitter", cx, 420, 22, C_STATION);
}

void dessiner_bilan(const t_simulation *sim)
{
    char texte[96];
    int cx = LARGEUR_FENETRE / 2, t, secondes = sim->stats.nb_ticks / TICKS_PAR_SECONDE;

    ClearBackground((Color) {35, 70, 45, 255});
    texte_centre("BILAN DE LA SIMULATION", cx, 60, 40, RAYWHITE);
    snprintf(texte, sizeof(texte), "Duree : %02d:%02d     Dechets apparus : %d",
             secondes / 60, secondes % 60, sim->stats.nb_apparus);
    texte_centre(texte, cx, 130, 22, RAYWHITE);
    for (t = 0; t < NB_TYPES_DECHETS; t++) {
        snprintf(texte, sizeof(texte), "%-9s : %d detectes, %d tries, %d restes au sol", NOM_TYPE[t],
                 sim->stats.nb_detectes[t], sim->stats.nb_collectes[t],
                 nb_dechets_en_attente_type(sim, (t_type_dechet) t));
        texte_centre(texte, cx, 190 + t * 34, 22, C_TYPE[t]);
    }
    snprintf(texte, sizeof(texte), "Recharges : %d     Pannes : %d", sim->stats.nb_recharges, sim->stats.nb_pannes);
    texte_centre(texte, cx, 310, 22, RAYWHITE);
    texte_centre("Entree : rejouer      Echap : quitter", cx, 420, 22, C_STATION);
}
