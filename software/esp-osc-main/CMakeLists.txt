cmake_minimum_required(VERSION 3.18)
project(esp_osc)

#idf_component_register(SRCS main.cpp mstrpck.cpp INCLUDE_DIRS "")

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
list(APPEND EXTRA_COMPONENT_DIRS components/esp-osc)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

#set(srcs
#    "components/esp-osc/esp_osc.c"
#    "components/tinyosc/tinyosc.c"
#)

#idf_component_register(SRCS ${srcs}
#    INCLUDE_DIRS "."
#)
