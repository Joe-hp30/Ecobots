/*
 * saisie.c — Clavier et souris (partie IHM)
 *
 * Sous Windows : API console de Windows (lecture des touches et des clics sans bloquer).
 * Sous macOS / Linux : termios (touche par touche, sans attendre Entrée) et le mode souris
 * des terminaux (séquences xterm SGR 1006).
 * Le reste du programme ne voit que lire_commande() et ne dépend donc pas du système.
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L   /* pour nanosleep() en C99 */
#endif

#include <stdio.h>
#include "saisie.h"

/* Codes internes renvoyés par lire_evenement() */
#define RIEN          -1
#define TOUCHE_TAB     9
#define TOUCHE_ECHAP  27
#define FLECHE_HAUT   1001
#define FLECHE_BAS    1002
#define FLECHE_GAUCHE 1003
#define FLECHE_DROITE 1004
#define CLIC_GAUCHE   1005

#ifdef _WIN32
/* =========================== Windows =========================== */
#include <windows.h>

static HANDLE entree;
static DWORD mode_origine;

void saisie_initialiser(void)
{
    entree = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(entree, &mode_origine);
    /* Souris activée ; "édition rapide" désactivée pour que les clics arrivent au programme */
    SetConsoleMode(entree, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
}

void saisie_terminer(void)
{
    SetConsoleMode(entree, mode_origine);
}

void attendre_ms(int ms)
{
    Sleep((DWORD) ms);
}

/* Lit un événement sans bloquer : touche, clic gauche, ou RIEN */
static int lire_evenement(int *x, int *y)
{
    INPUT_RECORD ev;
    DWORD nb = 0, lus = 0;

    while (GetNumberOfConsoleInputEvents(entree, &nb) && nb > 0) {
        if (!ReadConsoleInput(entree, &ev, 1, &lus) || lus == 0) {
            return RIEN;
        }
        if (ev.EventType == KEY_EVENT && ev.Event.KeyEvent.bKeyDown) {
            switch (ev.Event.KeyEvent.wVirtualKeyCode) {
                case VK_UP:     return FLECHE_HAUT;
                case VK_DOWN:   return FLECHE_BAS;
                case VK_LEFT:   return FLECHE_GAUCHE;
                case VK_RIGHT:  return FLECHE_DROITE;
                case VK_TAB:    return TOUCHE_TAB;
                case VK_ESCAPE: return TOUCHE_ECHAP;
                default:
                    if (ev.Event.KeyEvent.uChar.AsciiChar != 0) {
                        return (unsigned char) ev.Event.KeyEvent.uChar.AsciiChar;
                    }
            }
        } else if (ev.EventType == MOUSE_EVENT
                   && ev.Event.MouseEvent.dwEventFlags == 0
                   && (ev.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
            CONSOLE_SCREEN_BUFFER_INFO info;
            int haut = 0;
            if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
                haut = info.srWindow.Top;   /* la fenêtre peut être défilée dans le tampon */
            }
            *x = ev.Event.MouseEvent.dwMousePosition.X;
            *y = ev.Event.MouseEvent.dwMousePosition.Y - haut;
            return CLIC_GAUCHE;
        }
    }
    return RIEN;
}

#else
/* ======================== macOS / Linux ======================== */
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

static struct termios reglages_origine;
static int mode_brut_actif = 0;

/* Si le programme est interrompu (Ctrl+C, fenêtre fermée...), on remet quand même
 * le terminal en état avant de quitter : sinon il resterait sans écho et en mode souris. */
static void interruption(int signal_recu)
{
    (void) signal_recu;
    saisie_terminer();
    _exit(EXIT_FAILURE);
}

/* Terminal en mode "touche par touche", sans écho, lecture non bloquante, souris activée */
void saisie_initialiser(void)
{
    struct termios brut;

    if (tcgetattr(STDIN_FILENO, &reglages_origine) == 0) {
        brut = reglages_origine;
        brut.c_lflag &= ~(ICANON | ECHO);
        brut.c_cc[VMIN] = 0;    /* read() rend la main tout de suite... */
        brut.c_cc[VTIME] = 0;   /* ...même si aucune touche n'a été tapée */
        tcsetattr(STDIN_FILENO, TCSANOW, &brut);
        mode_brut_actif = 1;
    }
    printf("\033[?1000h\033[?1006h\033[?25l");   /* clics souris (SGR) + curseur caché */
    fflush(stdout);

    signal(SIGINT, interruption);    /* Ctrl+C */
    signal(SIGTERM, interruption);
    signal(SIGHUP, interruption);    /* fenêtre du Terminal fermée */
}

/* Remet le terminal comme avant : à faire impérativement en quittant */
void saisie_terminer(void)
{
    printf("\033[0m\033[?1000l\033[?1006l\033[?25h");   /* couleurs, souris, curseur : comme avant */
    fflush(stdout);
    if (mode_brut_actif) {
        tcsetattr(STDIN_FILENO, TCSANOW, &reglages_origine);
        mode_brut_actif = 0;
    }
}

void attendre_ms(int ms)
{
    struct timespec duree;

    duree.tv_sec = ms / 1000;
    duree.tv_nsec = (long) (ms % 1000) * 1000000L;
    nanosleep(&duree, NULL);
}

/* Lit un octet en attendant au plus 0,1 s (suite d'une séquence). Renvoie -1 si rien. */
static int lire_octet_rapide(void)
{
    unsigned char octet;
    int essai;

    for (essai = 0; essai < 10; essai++) {
        if (read(STDIN_FILENO, &octet, 1) == 1) {
            return octet;
        }
        attendre_ms(10);
    }
    return -1;
}

/* Lit un nombre décimal dans une séquence souris ; *fin reçoit le caractère qui le termine */
static int lire_nombre(int *fin)
{
    int c, n = 0;

    while ((c = lire_octet_rapide()) >= '0' && c <= '9') {
        n = n * 10 + (c - '0');
    }
    *fin = c;
    return n;
}

/* Lit un événement sans bloquer : touche, clic gauche, ou RIEN.
 * Flèches : ÉCHAP [ A/B/C/D. Clic : ÉCHAP [ < bouton ; x ; y M (x, y à partir de 1). */
static int lire_evenement(int *x, int *y)
{
    unsigned char octet;
    int suite, bouton, col, lig, fin;

    if (read(STDIN_FILENO, &octet, 1) != 1) {
        return RIEN;
    }
    if (octet != TOUCHE_ECHAP) {
        return octet;
    }

    suite = lire_octet_rapide();
    if (suite == -1) {
        return TOUCHE_ECHAP;   /* ÉCHAP seule */
    }
    if (suite != '[' && suite != 'O') {
        return RIEN;
    }

    suite = lire_octet_rapide();
    switch (suite) {
        case 'A': return FLECHE_HAUT;
        case 'B': return FLECHE_BAS;
        case 'D': return FLECHE_GAUCHE;
        case 'C': return FLECHE_DROITE;
        case '<':
            bouton = lire_nombre(&fin);
            col = lire_nombre(&fin);
            lig = lire_nombre(&fin);
            if (bouton == 0 && fin == 'M') {   /* bouton gauche enfoncé */
                *x = col - 1;
                *y = lig - 1;
                return CLIC_GAUCHE;
            }
            return RIEN;   /* relâchement ou autre bouton : ignoré */
        default:
            return RIEN;   /* séquence inconnue : ignorée */
    }
}

#endif

/* ===================== Commun aux deux systèmes ===================== */

t_commande lire_commande(int *x, int *y)
{
    switch (lire_evenement(x, y)) {
        case FLECHE_HAUT:   case 'z': case 'Z': return HAUT;
        case FLECHE_BAS:    case 's': case 'S': return BAS;
        case FLECHE_GAUCHE: case 'q': case 'Q': return GAUCHE;
        case FLECHE_DROITE: case 'd': case 'D': return DROITE;
        case CLIC_GAUCHE:                       return CLIC;
        case TOUCHE_TAB:                        return CHANGER_ENGIN;
        case TOUCHE_ECHAP:                      return QUITTER;
        default:                                return AUCUNE;  /* rien, ou touche inutile */
    }
}

/* Attend la prochaine touche (les clics sont ignorés) */
static int attendre_evenement_touche(void)
{
    int x, y, ev;

    for (;;) {
        ev = lire_evenement(&x, &y);
        if (ev != RIEN && ev != CLIC_GAUCHE) {
            return ev;
        }
        attendre_ms(20);
    }
}

/* Lit un chiffre entre min et max. Toute autre touche est ignorée :
 * impossible de faire planter le menu avec une lettre. */
int lire_choix_menu(int min, int max)
{
    int touche;

    do {
        touche = attendre_evenement_touche();
    } while (touche < '0' + min || touche > '0' + max);

    return touche - '0';
}

void attendre_touche(void)
{
    attendre_evenement_touche();
}
