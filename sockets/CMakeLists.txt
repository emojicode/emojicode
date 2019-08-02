file(GLOB SOURCES "*.cpp" "*.c")
file(GLOB EMOJIC_DEPEND "*.🍇")

get_filename_component(MAIN_FILE sockets.🍇 ABSOLUTE)
set(PACKAGE_FILE sockets.o)

add_library(sockets STATIC ${SOURCES} ${PACKAGE_FILE})
set_property(TARGET sockets PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(sockets PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
add_custom_command(OUTPUT ${PACKAGE_FILE} COMMAND emojicodec -p sockets -o ${PACKAGE_FILE} --color
        -S ${CMAKE_BINARY_DIR} -c ${MAIN_FILE} DEPENDS emojicodec s ${EMOJIC_DEPEND})
