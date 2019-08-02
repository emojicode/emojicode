file(GLOB S_SOURCES "*.cpp" "*.c" "*.hpp" "*.h")
file(GLOB S_EXCLUDED "utf8proc_data.c")

list(REMOVE_ITEM S_SOURCES ${S_EXCLUDED})

get_filename_component(MAIN_FILE s.🍇 ABSOLUTE)
file(GLOB EMOJIC_DEPEND "*.🍇")

add_library(s STATIC ${S_SOURCES} s.o)
set_property(TARGET s PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(s PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
add_custom_command(OUTPUT s.o COMMAND emojicodec -p s -o s.o --color ${MAIN_FILE} -O -c
        DEPENDS emojicodec ${EMOJIC_DEPEND})
