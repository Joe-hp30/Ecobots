# EcoBots — Protection d'une réserve naturelle

Projet Informatique 1 — ING1 ECE, semestre 1 2026-2027.
Équipe : Joe Humler-Payet, Alexandre Augiers de Crémiers, Ossoan Faret, Corentin Destres.

Simulation en C (mode console) : un drone piloté au clavier détecte les déchets d'une réserve,
un robot les ramasse (jusqu'à 3 à la fois) et les rapporte à un centre de collecte.
Chaque engin a une batterie qui se recharge sur les stations `+`.

La carte fait 20 × 40 cases : agrandir la fenêtre du Terminal à au moins 115 colonnes × 26 lignes
(Terminal → Réglages → Profils → Fenêtre, ou tirer le coin de la fenêtre).

## Compiler et lancer

**macOS / Linux** (dans le Terminal, depuis ce dossier) :

```
make            # compile le jeu
./ecobots       # lance le jeu
make tests      # lance les tests automatiques du moteur
```

**Windows** : ouvrir le dossier dans CLion (il lit `CMakeLists.txt`), puis lancer la cible
`ecobots`, ou `tests_moteur` pour les tests. Dans CLion, cocher « Emulate terminal in the output console »
dans la configuration d'exécution pour que les touches soient lues correctement.

## Commandes

| Touche | Action |
| --- | --- |
| Flèches ou Z Q S D | déplacer l'engin actif |
| Tab | passer du drone au robot |
| Échap | terminer la simulation et voir le bilan |

Légende : `D` drone · `R` robot · `*` déchet détecté · `C` centre de collecte · `+` station de recharge · `.` libre
Obstacles naturels infranchissables : `#` forêt dense · `^` falaise · `~` plan d'eau

## Organisation du code

| Fichier | Rôle | Responsable |
| --- | --- | --- |
| `simulation.h/.c` | Moteur : données et règles. Aucun affichage, aucune saisie. | Joe (terrain, déplacements, détection), Alexandre (déchets, collecte) |
| `affichage.h/.c` | IHM : tout ce qui s'affiche | Ossoan |
| `saisie.h/.c` | IHM : clavier → `t_commande` (Windows avec `conio.h`, macOS/Linux avec `termios`) | Ossoan |
| `main.c` | Boucle principale : saisie → moteur → affichage | Tous |
| `tests.c` | Tests automatiques du moteur (plan de tests du rapport) | Corentin |

Pour passer en mode graphique (raylib) ou utiliser `affichage_console.h`, on ne remplace que
`affichage.c` et `saisie.c` : `simulation.c` ne change pas.

## État d'avancement

- [x] Jeu de base : affichage, pilotage du drone et du robot, détection sur 9 cases,
      apparition aléatoire, ramassage, dépôt, panneau d'infos, bilan
- [x] Tests automatiques du moteur (29 tests)
- [ ] Extension E1 : génération aléatoire de la réserve (Corentin)
- [ ] Extension E2 : types de déchets et statistiques (Corentin, affichage : Ossoan)
- [x] Extension E3 : batterie du drone et du robot, stations de recharge, pannes
- [x] Carte 20 × 40, 4 centres de collecte, robot de capacité 3

## Travailler avec Git

```
git pull                              # récupérer le travail des autres
git checkout -b nom-de-la-tache       # une branche par tâche
# ... coder, compiler sans avertissement, make tests ...
git add fichiers_modifies
git commit -m "Ce qui a changé"
git push -u origin nom-de-la-tache    # puis ouvrir une Pull Request sur GitHub
```

Règles : jamais de push direct sur `main` ; on ne fusionne que du code qui compile et passe les tests.

## Aide reçue

Le squelette du code (architecture, moteur de base, tests) a été écrit avec l'aide de Claude (IA, Anthropic),
à partir de notre conception de mi-parcours. À déclarer dans la partie « Sources et aide » du rapport final,
comme le demande le sujet. Chaque membre doit savoir expliquer tout le code à la soutenance.
