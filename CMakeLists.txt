cmake_minimum_required(VERSION 3.16)
project(JuicedEditor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3 では COMPONENTS の指定が必須です
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(juiced_editor main.cpp)

target_link_libraries(juiced_editor PRIVATE SFML::Graphics SFML::Window SFML::System)