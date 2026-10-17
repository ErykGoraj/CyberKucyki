cmake_minimum_required(VERSION 3.16) # Minimalna wersja CMake

project(CyberKucyki LANGUAGES CXX) # Nazwa projektu i język c++

include(FetchContent) # Dołączamy mechanizm pobierania zależności


# Biblioteki statyczne
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Elementy biblioteki SFML ktore wylaczamy
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

# Źródło biblioteki z github
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
)

# Jeśli potrzeba pobiera SFML i dodaje do projektu
FetchContent_MakeAvailable(SFML)
add_executable(gra main.cpp) # 1 cel budowania 2 i reszta plikow dajaca zbudowanie

target_compile_features(gra PRIVATE cxx_std_17) # Wymagamy conajmniej c++17

target_link_libraries(gra PRIVATE SFML::Graphics) # Łączy program z modułem SFML