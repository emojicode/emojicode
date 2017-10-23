file(GLOB RUNTIME "*")
add_library(runtime STATIC ${RUNTIME})
target_compile_options(runtime PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
