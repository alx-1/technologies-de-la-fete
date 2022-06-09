set(srcs
    "tinyosc.c"
)

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS "." 
)
