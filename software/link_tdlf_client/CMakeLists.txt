cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
list(APPEND EXTRA_COMPONENT_DIRS components/apa102)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(link_tdlf_client)
