cmake_minimum_required(VERSION 3.22.1)
project("forestgame")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(forestgame SHARED forest_game.cpp)

target_compile_options(forestgame PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(forestgame
    android
    GLESv3
    log
)
