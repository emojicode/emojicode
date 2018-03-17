file(GLOB RUNTIME "*")
add_library(runtime STATIC ${RUNTIME})
set_property(TARGET runtime PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(runtime PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
