cmake_minimum_required(VERSION 3.11)
project(packet_forwarder)

# depends on libloragw
# and `config.h`
set(LIBLORAGW_INC_DIR "../libloragw/inc")

# tinymt32 parson and base64
set(TOOLS_INC_DIR "../libtools/inc")
include_directories(${TOOLS_INC_DIR})
file(GLOB TOOLS_SRC_FILES "../libtools/src/*.c")

add_library(tools STATIC ${TOOLS_SRC_FILES})
include_directories(${TOOLS_INC_DIR})

set(INC_DIR inc)
include_directories(${INC_DIR})
file(GLOB SRC_FILES "src/*.c")
add_executable(packet_forwarder ${SRC_FILES})
target_include_directories(packet_forwarder PRIVATE ${LIBLORAGW_INC_DIR})
target_link_libraries(packet_forwarder loragw pthread rt m tools)
