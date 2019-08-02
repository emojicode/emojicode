file(GLOB EMOJIC_DEPEND "*.🍇")

# NOTE: CMake is absolutely not required for packages that do not link to non-Emojicode
# object code. CMake is used here only for automation of the build process of the entire project!

get_filename_component(MAIN_FILE json.🍇 ABSOLUTE)
set(PACKAGE_FILE json.o)

add_library(json STATIC ${PACKAGE_FILE})
set_property(TARGET json PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET json PROPERTY LINKER_LANGUAGE CXX)
target_compile_options(json PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
add_custom_command(OUTPUT ${PACKAGE_FILE} COMMAND emojicodec -p json -o ${PACKAGE_FILE} --color
-S ${CMAKE_BINARY_DIR} -c ${MAIN_FILE} -O DEPENDS emojicodec s ${EMOJIC_DEPEND})
