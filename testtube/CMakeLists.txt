file(GLOB EMOJIC_DEPEND "*.🍇")

# NOTE: CMake is absolutely not required for packages that do not link to non-Emojicode
# object code. CMake is used here only for automation of the build process of the entire project!

get_filename_component(MAIN_FILE testtube.🍇 ABSOLUTE)
set(PACKAGE_FILE testtube.o)

add_library(testtube STATIC ${PACKAGE_FILE})
set_property(TARGET testtube PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET testtube PROPERTY LINKER_LANGUAGE CXX)
target_compile_options(testtube PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
add_custom_command(OUTPUT ${PACKAGE_FILE} COMMAND emojicodec -p testtube -o ${PACKAGE_FILE} --color
-S ${CMAKE_BINARY_DIR} -c ${MAIN_FILE} DEPENDS emojicodec s ${EMOJIC_DEPEND})
