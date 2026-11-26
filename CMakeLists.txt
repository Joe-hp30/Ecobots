cmake_minimum_required(VERSION 3.10)
project(ecobots C)

set(CMAKE_C_STANDARD 99)

# Le jeu
add_executable(ecobots main.c simulation.c affichage.c saisie.c)

# Les tests du moteur (pas besoin de l'IHM)
add_executable(tests_moteur tests.c simulation.c)
