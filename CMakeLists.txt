cmake_minimum_required(VERSION 3.17)

project(luka)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

find_package(LLVM REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config)

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs core analysis native bitwriter
	OUTPUT_VARIABLE LLVM_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/third_party/include)

file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB_RECURSE LIBS    libs/*)

add_executable(luka ${SOURCES})

add_library(vector STATIC libs/vector.c)
target_link_libraries(luka vector)
target_link_libraries(luka ${LLVM_LIBRARIES})
target_compile_options(luka PRIVATE -Wall -Wextra -Wmost -Weverything -pedantic -Werror -Wno-padded)

add_custom_target(docs cd ${PROJECT_SOURCE_DIR}/docs
  COMMAND doxygen && cd -
  DEPENDS ${PROJECT_SOURCE_DIR}/docs/Doxyfile)

file(GLOB_RECURSE LUKA_LIB lib/*.luka)
file(GLOB_RECURSE LUKA_STDLIB lib/std/*.luka)
install(TARGETS luka DESTINATION bin)
install(FILES ${LUKA_LIB} DESTINATION lib/luka)
install(FILES ${LUKA_STDLIB} DESTINATION lib/luka/std)

enable_testing()
add_subdirectory(tests)
