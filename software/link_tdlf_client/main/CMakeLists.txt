idf_component_register(SRCS main.cpp)
#set(srcs "ESP32APA102Driver.c" "apa102LEDStrip.c" "colourObject.c")
set(srcs "main.cpp" "apa102LEDStrip.c" "colourObject.c")
target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)