file(GLOB SOURCES "*.cpp" "*.c")
file(GLOB EMOJIC_DEPEND "*.emojic")

get_filename_component(MAIN_FILE sockets.emojic ABSOLUTE)
set(PACKAGE_FILE sockets.o)

add_library(sockets STATIC ${SOURCES} ${PACKAGE_FILE})
set_property(TARGET files PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(files PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
add_custom_command(OUTPUT ${PACKAGE_FILE} COMMAND emojicodec -p sockets -o ${PACKAGE_FILE} -i interface.emojii --color
        -S ${CMAKE_BINARY_DIR} ${MAIN_FILE} DEPENDS emojicodec s ${EMOJIC_DEPEND})
