cmake_minimum_required(VERSION 3.5)
project(Emojicode)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

add_compile_options(-pedantic-errors)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-fcolor-diagnostics)
endif()

if(defaultPackagesDirectory)
  add_definitions(-DdefaultPackagesDirectory="${defaultPackagesDirectory}")
endif()

add_subdirectory(runtime)
add_subdirectory(s)
add_subdirectory(files)
add_subdirectory(Compiler)
add_subdirectory(sockets)
add_subdirectory(testtube)

add_custom_target(dist python3 ${PROJECT_SOURCE_DIR}/dist.py ${PROJECT_SOURCE_DIR})
add_custom_target(tests python3 ${PROJECT_SOURCE_DIR}/tests.py ${PROJECT_SOURCE_DIR})
add_custom_target(magicinstall python3 ${PROJECT_SOURCE_DIR}/dist.py ${PROJECT_SOURCE_DIR} install)
