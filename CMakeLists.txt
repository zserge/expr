cmake_minimum_required(VERSION 3.0.2)

project(expr C)

set(CMAKE_C_STANDARD 99)

option(EXPR_PICKY_COMPILER "Enable picky compiler options" ON)

if(EXPR_PICKY_COMPILER)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -Wno-unused-function -Wpedantic -Wdeclaration-after-statement -Wstrict-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline"
  )
endif()

include_directories(include)

add_executable(example1 examples/example1.c)
target_link_libraries(example1 m)

add_executable(example2 examples/example2.c)
target_link_libraries(example2 m)

add_executable(example3 examples/example3.c)
target_link_libraries(example3 m)

add_executable(test expr_test.c)
target_link_libraries(test m)
