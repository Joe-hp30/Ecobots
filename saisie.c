/*
 * saisie.c — Lecture du clavier (partie IHM)
 *
 * Sous Windows : conio.h (_getch), comme demandé dans le sujet.
 * Sous macOS / Linux : termios, pour lire une touche sans attendre Entrée.
 * Le reste du programme ne voit que lire_commande() et ne dépend donc pas du système.
 */

#include <stdio.h>
#include "saisie.h"

/* Codes de touches communs */
#define TOUCHE_TAB     9
#define TOUCHE_ECHAP  27

/* Codes internes pour les flèches, renvoyés par lire_touche() */
#define FLECHE_HAUT   1001
#define FLECHE_BAS    1002
#define FLECHE_GAUCHE 1003
#define FLECHE_DROITE 1004

#ifdef _WIN32
/* =========================== Windows =========================== */
#include <conio.h>

void saisie_initialiser(void) { }
void saisie_terminer(void) { }

/* Lit une touche. Les flèches envoient deux codes : 0 ou 224, puis le code de la flèche. */
static int lire_touche(void)
{
    int c = _getch();

    if (c == 0 || c == 224) {
        switch (_getch()) {
            case 72: return FLECHE_HAUT;
            case 80: return FLECHE_BAS;
            case 75: return FLECHE_GAUCHE;
            case 77: return FLECHE_DROITE;
            default: return 0;   /* autre touche spéciale (F1...) : ignorée */
        }
    }
    return c;
}

#else
/* ======================== macOS / Linux ======================== */
#include <termios.h>
#include <unistd.h>

static struct termios reglages_origine;
static int mode_brut_actif = 0;

/* Passe le terminal en mode "touche par touche" sans écho */
void saisie_initialiser(void)
{
    struct termios brut;

    if (tcgetattr(STDIN_FILENO, &reglages_origine) != 0) {
        return;   /* pas un vrai terminal : on laisse tel quel */
    }
    brut = reglages_origine;
    brut.c_lflag &= ~(ICANON | ECHO);
    brut.c_cc[VMIN] = 1;
    brut.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &brut);
    mode_brut_actif = 1;
}

/* Remet le terminal comme avant : à faire impérativement en quittant */
void saisie_terminer(void)
{
    if (mode_brut_actif) {
        tcsetattr(STDIN_FILENO, TCSANOW, &reglages_origine);
        mode_brut_actif = 0;
    }
}

/* Lit un octet en attendant au plus 0,1 s. Renvoie -1 si rien n'est arrivé. */
static int lire_octet_rapide(void)
{
    struct termios t, ancien;
    unsigned char octet;
    int lu;

    tcgetattr(STDIN_FILENO, &ancien);
    t = ancien;
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    lu = (int) read(STDIN_FILENO, &octet, 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &ancien);
    return (lu == 1) ? octet : -1;
}

/* Lit une touche. Les flèches envoient 3 octets : ÉCHAP, '[', puis A/B/C/D. */
static int lire_touche(void)
{
    unsigned char octet;
    int suite;

    if (read(STDIN_FILENO, &octet, 1) != 1) {
        return TOUCHE_ECHAP;   /* entrée fermée : on considère qu'on quitte */
    }
    if (octet != TOUCHE_ECHAP) {
        return octet;
    }

    suite = lire_octet_rapide();
    if (suite == -1) {
        return TOUCHE_ECHAP;   /* ÉCHAP seule */
    }
    if (suite == '[' || suite == 'O') {
        switch (lire_octet_rapide()) {
            case 'A': return FLECHE_HAUT;
            case 'B': return FLECHE_BAS;
            case 'D': return FLECHE_GAUCHE;
            case 'C': return FLECHE_DROITE;
            default:  return 0;
        }
    }
    return 0;   /* séquence inconnue : ignorée */
}

#endif

/* ===================== Commun aux deux systèmes ===================== */

t_commande lire_commande(void)
{
    switch (lire_touche()) {
        case FLECHE_HAUT:   case 'z': case 'Z': return HAUT;
        case FLECHE_BAS:    case 's': case 'S': return BAS;
        case FLECHE_GAUCHE: case 'q': case 'Q': return GAUCHE;
        case FLECHE_DROITE: case 'd': case 'D': return DROITE;
        case TOUCHE_TAB:                        return CHANGER_ENGIN;
        case TOUCHE_ECHAP:                      return QUITTER;
        default:                                return AUCUNE;  /* touche inutile */
    }
}

/* Lit un chiffre entre min et max. Toute autre touche est ignorée : impossible
 * de faire planter le menu avec une lettre, comme ce serait le cas avec un scanf mal protégé. */
int lire_choix_menu(int min, int max)
{
    int touche;

    do {
        touche = lire_touche();
    } while (touche < '0' + min || touche > '0' + max);

    return touche - '0';
}

void attendre_touche(void)
{
    lire_touche();
}
