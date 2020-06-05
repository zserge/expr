cmake_minimum_required(VERSION 3.0.2)

project(expr C)

set(CMAKE_C_STANDARD 99)

option(EXPR_PICKY_COMPILER "Enable picky compiler options" ON)

if(NOT MSVC)
  option(EXPR_PVS_STUDIO "Enable PVS-Studio analysis" OFF)
endif()

#TODO: coverage

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if(EXPR_PICKY_COMPILER)
  if(NOT MSVC)
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -Wall -Werror -Wno-unused-function -Wpedantic -Wdeclaration-after-statement -Wstrict-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline"
    )
  endif()
endif()

include_directories(include)

if(NOT MSVC)
  list(APPEND _libs m)
endif()

add_executable(example1 examples/example1.c)
target_link_libraries(example1 ${_libs})

add_executable(example2 examples/example2.c)
target_link_libraries(example2 ${_libs})

add_executable(example3 examples/example3.c)
target_link_libraries(example3 ${_libs})

add_executable(test expr_test.c)
target_link_libraries(test ${_libs})

if(EXPR_PVS_STUDIO)
  include(PVS-Studio)
  if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  endif()
  pvs_studio_add_target(
    TARGET
    pvs_studio_analysis
    ALL
    FORMAT
    fullhtml
    ANALYZE
    example1
    example2
    example3
    test
    LOG
    pvs_studio_fullhtml)
endif()
