cmake_minimum_required(VERSION 3.14)
project(ecobots C)

set(CMAKE_C_STANDARD 99)

# ---------- Version console ----------
add_executable(ecobots main.c simulation.c affichage.c saisie.c)

# ---------- Tests du moteur (pas besoin de l'IHM) ----------
add_executable(tests_moteur tests.c simulation.c)

# ---------- Version graphique (raylib) ----------
# Si raylib n'est pas déjà installé sur l'ordinateur, CMake le télécharge et le compile
# automatiquement (il faut Internet la première fois, et 1 à 2 minutes de compilation).
option(ECOBOTS_GRAPHIQUE "Compiler la version graphique raylib" ON)

if (ECOBOTS_GRAPHIQUE)
    find_package(raylib 5.0 QUIET)
    if (NOT raylib_FOUND)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(raylib)
    endif ()

    add_executable(ecobots_graphique main_raylib.c simulation.c affichage_raylib.c)
    target_link_libraries(ecobots_graphique raylib)
    if (APPLE)
        target_link_libraries(ecobots_graphique "-framework OpenGL" "-framework Cocoa"
                              "-framework IOKit" "-framework CoreVideo")
    endif ()
endif ()
