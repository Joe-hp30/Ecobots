# Makefile EcoBots — macOS / Linux (sous Windows, CLion utilise CMakeLists.txt)
#   make            compile la version console      ->  ./ecobots
#   make graphique  compile la version raylib       ->  ./ecobots_graphique
#   make tests      compile et lance les tests du moteur
#   make clean      supprime les fichiers compilés
#
# Version graphique sur Mac : installer raylib une fois avec  brew install raylib

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -g

SOURCES_CONSOLE   = main.c simulation.c affichage.c saisie.c
SOURCES_GRAPHIQUE = main_raylib.c simulation.c affichage_raylib.c

# Options pour raylib : Homebrew sur Mac, paquets du système sous Linux.
# On peut les remplacer :  make graphique RAYLIB="-I... -L... -lraylib ..."
ifeq ($(shell uname),Darwin)
    BREW   := $(shell brew --prefix 2>/dev/null)
    RAYLIB ?= -I$(BREW)/include -L$(BREW)/lib -lraylib \
              -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    RAYLIB ?= -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

ecobots: $(SOURCES_CONSOLE) simulation.h affichage.h saisie.h
	$(CC) $(CFLAGS) -o ecobots $(SOURCES_CONSOLE)

graphique: $(SOURCES_GRAPHIQUE) simulation.h affichage_raylib.h
ifeq ($(shell uname),Darwin)
	@test -n "$(BREW)" || { echo ""; echo "Homebrew n'est pas installe : voir la section 'Version graphique' du README."; echo ""; exit 1; }
	@test -f "$(BREW)/include/raylib.h" || { echo ""; echo "raylib n'est pas installe. Tapez :  brew install raylib"; echo ""; exit 1; }
endif
	$(CC) $(CFLAGS) -o ecobots_graphique $(SOURCES_GRAPHIQUE) $(RAYLIB)

tests: tests.c simulation.c simulation.h
	$(CC) $(CFLAGS) -o lancer_tests tests.c simulation.c
	./lancer_tests

clean:
	rm -f ecobots ecobots_graphique lancer_tests

.PHONY: graphique tests clean
