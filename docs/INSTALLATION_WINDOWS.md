# Installer EcoBots sur Windows

Deux cas : juste **jouer**, ou **travailler sur le code** avec Git et CLion.

---

## 1. Juste jouer (2 minutes)

1. Ouvrir la page https://github.com/Joe-hp30/Ecobots/releases/tag/derniere-version
2. Télécharger **`EcoBots_Windows.zip`** (GitHub le recompile tout seul à chaque modification du code).
3. Clic droit sur le fichier → **Extraire tout**.
4. Double-cliquer sur **`ecobots_graphique.exe`** (version graphique) ou **`ecobots.exe`** (version console).

Si Windows affiche « Windows a protégé votre ordinateur » : **Informations complémentaires** →
**Exécuter quand même**. C'est normal : le programme n'est pas signé.

---

## 2. Travailler sur le code (une seule fois, environ 15 minutes)

### Étape 1 — Accepter l'invitation GitHub

Joe vous ajoute comme collaborateur : vous recevez un **mail de GitHub**. Cliquer sur **View invitation**
puis **Accept invitation**. Sans ça, vous pouvez lire le code mais pas envoyer vos modifications.

### Étape 2 — Installer Git

1. Télécharger Git sur https://git-scm.com/download/win (le téléchargement démarre tout seul).
2. Lancer l'installateur et cliquer sur **Next** à chaque écran : les choix par défaut conviennent.

### Étape 3 — Dire à Git qui vous êtes

Ouvrir **Git Bash** (menu Démarrer) et taper, avec **vos** informations :

```
git config --global user.name "Prénom Nom"
git config --global user.email "VOTRE-PSEUDO@users.noreply.github.com"
```

L'adresse exacte se trouve sur GitHub : photo de profil → **Settings** → **Emails**
(cocher « Keep my email addresses private », l'adresse en `@users.noreply.github.com` s'affiche).
Comme le dépôt est public, cela évite que votre vraie adresse soit visible.

### Étape 4 — Récupérer le projet

Toujours dans Git Bash :

```
cd ~/Documents
git clone https://github.com/Joe-hp30/Ecobots.git
```

Le dossier `Documents\Ecobots` est créé avec tout le projet.

### Étape 5 — Ouvrir le projet dans CLion

1. CLion → **Open** → choisir le dossier `Documents\Ecobots` → **Trust Project**.
2. CLion lit le fichier `CMakeLists.txt`. **La première fois**, il télécharge et compile raylib tout seul :
   compter 1 à 2 minutes (il faut Internet). Attendre la fin dans l'onglet **CMake** en bas.
3. En haut à droite, la liste déroulante propose 3 programmes :
   - **`ecobots_graphique`** : la version graphique → choisir puis cliquer sur ▶ ;
   - **`ecobots`** : la version console (voir l'étape 6) ;
   - **`tests_moteur`** : les tests automatiques du moteur → ▶ (doit afficher « 0 echec »).

### Étape 6 — Lancer la version console

La fenêtre « Run » de CLion n'est pas une vraie console : les flèches et la souris n'y marchent pas.
Après l'avoir compilée une fois avec ▶, la lancer depuis l'onglet **Terminal** de CLion (en bas) :

```
cmake-build-debug\ecobots.exe
```

Agrandir la fenêtre à au moins 125 colonnes pour voir tout le panneau.

### Étape 7 — Premier envoi sur GitHub

La première fois que vous faites `git push`, une fenêtre **« Connect to GitHub »** s'ouvre :
cliquer sur **Sign in with your browser** et se connecter. C'est fait une fois pour toutes.

---

## 3. Au quotidien

Dans le Terminal de CLion ou dans Git Bash, depuis le dossier `Ecobots` :

```
git pull                         # AVANT de coder : récupérer le travail des autres
# ... coder, compiler, tester (tests_moteur) ...
git add .
git commit -m "Ce que j'ai fait"
git push                         # envoyer sur GitHub
```

Chacun travaille sur ses fichiers (voir la répartition dans le README) pour éviter les conflits.

---

## En cas de problème

| Problème | Solution |
| --- | --- |
| `git` n'est pas reconnu | Fermer et rouvrir CLion (ou Git Bash) après avoir installé Git |
| CMake n'arrive pas à télécharger raylib | Vérifier la connexion Internet et que Git est installé, puis **File → Reload CMake Project** |
| `git push` refusé (403 / permission denied) | L'invitation GitHub n'a pas été acceptée (étape 1) |
| `git push` refusé (rejected, fetch first) | Faire `git pull` d'abord, puis recommencer `git push` |
| Les flèches ne marchent pas en console | Lancer `ecobots.exe` depuis le Terminal, pas avec ▶ (étape 6) |
