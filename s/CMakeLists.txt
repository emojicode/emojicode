file(GLOB S_SOURCES "*.cpp" "*.c")

get_filename_component(MAIN_FILE s.emojic ABSOLUTE)

add_library(s STATIC ${S_SOURCES} s.o)
target_compile_options(s PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
target_link_libraries(s dl pthread)
add_custom_command(OUTPUT s.o COMMAND emojicodec -p s -o s.o -i interface.emojii ${MAIN_FILE})
