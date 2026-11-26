cmake_minimum_required(VERSION 3.10)
project(ecobots C)

set(CMAKE_C_STANDARD 99)

# Le jeu en mode console
add_executable(ecobots main.c simulation.c affichage.c saisie.c)

# Les tests du moteur (pas besoin de l'IHM)
add_executable(tests_moteur tests.c simulation.c)

# Le jeu en mode graphique, seulement si raylib est installé
find_package(raylib QUIET)
if (raylib_FOUND)
    add_executable(ecobots_graphique main_raylib.c simulation.c affichage_raylib.c)
    target_link_libraries(ecobots_graphique raylib)
    if (APPLE)
        target_link_libraries(ecobots_graphique "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
    endif ()
else ()
    message(STATUS "raylib introuvable : la version graphique ne sera pas compilee")
endif ()
