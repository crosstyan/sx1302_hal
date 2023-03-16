cmake_minimum_required(VERSION 3.11)
project(libloragw)
set(INC_DIR inc)
set(TOOLS_INC_DIR "../libtools/inc")
file(GLOB TOOLS_SRC_FILES "../libtools/src/*.c")
file(GLOB SRC_FILES "src/*.c")
file(GLOB TST_SRC_FILES "tst/*.c")

# No need lib prefix for libraries
add_library(loragw STATIC ${SRC_FILES})
include_directories(${INC_DIR})
add_library(tools STATIC ${TOOLS_SRC_FILES})
include_directories(${TOOLS_INC_DIR})

# build exe for each test file
foreach(test ${TST_SRC_FILES})
  get_filename_component(test_name ${test} NAME_WE)
  add_executable(${test_name} ${test})
  # You need `m` (for `-lm` i.e. math) to link the test executables
  target_link_libraries(${test_name} PRIVATE loragw tools m)
endforeach()

# https://stackoverflow.com/questions/10487256/cmake-how-to-properly-copy-static-librarys-header-file-into-usr-include
export(TARGETS loragw FILE loragw.cmake)
file(GLOB INC_FILES ${INC_DIR}/*.h)
list(FILTER INC_FILES INCLUDE REGEX "loragw_.*$")
# message(STATUS "INC_FILES: ${INC_FILES}")
set_target_properties(loragw PROPERTIES PUBLIC_HEADER "${INC_FILES}")

# https://visualgdb.com/tutorials/linux/cmake/find_package/
install(TARGETS loragw
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)
