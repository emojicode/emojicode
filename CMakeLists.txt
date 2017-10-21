cmake_minimum_required(VERSION 3.2.2)
project(Emojicode)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
add_compile_options(-fcolor-diagnostics)
add_compile_options(-fcolor-diagnostics)
endif()

if(defaultPackagesDirectory)
  add_definitions(-DdefaultPackagesDirectory="${defaultPackagesDirectory}")
endif()

add_subdirectory(runtime)
add_subdirectory(s)
add_subdirectory(Compiler)

file(GLOB EMOJICODEMIG_SOURCES "Migrator/*")
add_executable(emojicodemig ${EMOJICODEMIG_SOURCES})
target_compile_options(emojicodemig PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)

# find_library(SYSTEM_ALLEGRO NAMES allegro)
# target_link_libraries(allegro ${SYSTEM_ALLEGRO} allegro_color allegro_primitives allegro_image allegro_ttf allegro_audio allegro_acodec)

add_custom_target(dist python3 ${PROJECT_SOURCE_DIR}/dist.py ${PROJECT_SOURCE_DIR})
add_custom_target(tests python3 ${PROJECT_SOURCE_DIR}/tests.py ${PROJECT_SOURCE_DIR})
add_custom_target(magicinstall python3 ${PROJECT_SOURCE_DIR}/dist.py ${PROJECT_SOURCE_DIR} install)
