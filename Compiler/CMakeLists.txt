find_package(LLVM 8.0 REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE EMOJICODEC_SOURCES "*")
add_executable(emojicodec ${EMOJICODEC_SOURCES})
target_compile_options(emojicodec PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)

llvm_map_components_to_libnames(LLVM_LIBS core codegen passes ${LLVM_TARGETS_TO_BUILD})
target_link_libraries(emojicodec z m ${LLVM_LIBS})
