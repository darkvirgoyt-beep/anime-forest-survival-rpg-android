cmake_minimum_required(VERSION 3.22.1)
project("forestgame")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(forestgame SHARED
    forest_game.cpp
    physics/physics.cpp
    controller/third_person_controller.cpp
    combat/combat_system.cpp
    rpg/progression.cpp
)

target_compile_options(forestgame PRIVATE -Wall -Wextra -Wpedantic)
# NDK r27 and lower need explicit 16 KB ELF alignment; keep these flags
# explicit so the library remains aligned even if a CI image is downgraded.
target_link_options(forestgame PRIVATE
    "-Wl,-z,max-page-size=16384"
    "-Wl,-z,common-page-size=16384"
)
target_include_directories(forestgame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(forestgame
    android
    GLESv3
    log
)
