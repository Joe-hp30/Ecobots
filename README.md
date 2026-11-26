# EcoBots — Protection d'une réserve naturelle

Projet Informatique 1 — ING1 ECE Paris, semestre 1 2026-2027.

**Équipe :** Joe Humler-Payet, Alexandre Augiers de Crémiers, Ossoan Faret, Corentin Destres.

Simulation en C (mode console) de la surveillance d'une réserve naturelle : un **drone** détecte les
déchets abandonnés, un **robot** les ramasse et les rapporte à un centre de collecte. Les deux engins
fonctionnent sur batterie ; en cas de panne, un **technicien** vient les chercher à pied.

## Lancer le jeu

**macOS / Linux**, dans le Terminal, depuis ce dossier :

```
make            # compile le jeu
./ecobots       # lance le jeu
make tests      # compile et lance les tests automatiques du moteur
```

La carte fait 20 × 40 cases : agrandir la fenêtre du Terminal à au moins **115 colonnes × 26 lignes**.

**Windows** : ouvrir le dossier dans CLion (il lit `CMakeLists.txt`) et lancer la cible `ecobots`
(ou `tests_moteur`). Dans la configuration d'exécution, cocher « Emulate terminal in the output console ».

## Règles

- La réserve contient des obstacles naturels infranchissables : forêts denses `#`, falaises `^`, plans d'eau `~`.
- Des déchets apparaissent au hasard et restent invisibles tant que le drone ne les a pas détectés :
  il faut qu'il passe sur leur case ou sur une des 8 cases voisines.
- Le robot ramasse les déchets détectés en passant dessus (jusqu'à **3** à la fois),
  puis les vide tous en passant sur un centre de collecte `C`.
- **Batteries** (en %) : le drone use 2 % par case ; le robot 1 % à vide, plus 1 % par déchet transporté.
  Sur une station de recharge `+`, chaque tour rend 25 %. Une alerte s'affiche sous 20 %.
- **Panne** : à 0 %, l'engin s'arrête sur place. Un technicien `H` part à pied de la station la plus proche
  (il contourne les obstacles), le récupère, puis le ramène à la station où il est rechargé.
  Pendant ce temps, on pilote l'autre engin ou on attend.

## Commandes

| Touche | Action |
| --- | --- |
| Flèches ou Z Q S D | déplacer l'engin piloté |
| Espace | attendre un tour (recharge, arrivée du technicien) |
| Tab | passer du drone au robot |
| Échap | terminer la simulation et voir le bilan |

Légende : `D` drone · `R` robot · `H` technicien · `*` déchet détecté · `C` centre de collecte ·
`+` station de recharge · `.` libre · engin sur fond rouge = en panne

## Organisation du dépôt

```
Ecobots/
├── main.c            boucle principale : saisie → moteur → affichage
├── simulation.h/.c   moteur : données et règles (aucun affichage, aucune saisie)
├── affichage.h/.c    IHM : tout ce qui s'affiche
├── saisie.h/.c       IHM : clavier → t_commande (Windows : conio.h, macOS/Linux : termios)
├── tests.c           tests automatiques du moteur
├── Makefile          compilation macOS / Linux
├── CMakeLists.txt    compilation Windows (CLion)
└── docs/
    └── EcoBots_Rendu_mi-parcours.pdf
```

Le moteur ne dépend pas de l'IHM : pour passer en mode graphique (raylib) ou utiliser
`affichage_console.h`, on ne remplace que `affichage.c` et `saisie.c`.

## Répartition

| Membre | Responsabilité |
| --- | --- |
| Joe Humler-Payet | Moteur : terrain, déplacements, détection |
| Alexandre Augiers de Crémiers | Moteur : déchets et collecte |
| Ossoan Faret | IHM : affichage, clavier, menu |
| Corentin Destres | Extensions, tests |

## Avancement

- [x] Jeu de base : affichage, pilotage du drone et du robot, détection sur 9 cases,
      apparition aléatoire, ramassage, dépôt, panneau d'infos, bilan
- [x] Obstacles naturels variés : forêts, falaises, plans d'eau
- [x] Extension E3 : batteries, stations de recharge, pannes avec intervention d'un technicien
- [x] Robot de capacité 3, carte 20 × 40 avec 4 centres et 5 stations
- [x] Tests automatiques du moteur (`make tests`)
- [ ] Extension E1 : génération aléatoire de la réserve
- [ ] Extension E2 : types de déchets et statistiques

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
