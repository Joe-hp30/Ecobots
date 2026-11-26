# Makefile EcoBots — macOS / Linux (sous Windows, CLion utilise CMakeLists.txt)
#   make         compile le jeu        ->  ./ecobots
#   make tests   compile et lance les tests du moteur
#   make clean   supprime les fichiers compilés

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -g

SOURCES_JEU = main.c simulation.c affichage.c saisie.c

ecobots: $(SOURCES_JEU) simulation.h affichage.h saisie.h
	$(CC) $(CFLAGS) -o ecobots $(SOURCES_JEU)

tests: tests.c simulation.c simulation.h
	$(CC) $(CFLAGS) -o lancer_tests tests.c simulation.c
	./lancer_tests

clean:
	rm -f ecobots lancer_tests

.PHONY: tests clean
