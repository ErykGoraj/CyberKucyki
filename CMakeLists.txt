cmake_minimum_required(VERSION 3.16)

project(CyberKucyki LANGUAGES CXX)

include(FetchContent)



set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)


FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
)


FetchContent_MakeAvailable(SFML)
add_executable(gra main.cpp)

target_compile_features(gra PRIVATE cxx_std_17)

target_link_libraries(gra PRIVATE SFML::Graphics SFML::Audio)

if(MSVC)
    target_compile_options(gra PRIVATE /utf-8)
endif()

