cmake_minimum_required(VERSION 3.17)

project(luka)

# find_package(LLVM REQUIRED analysis bitwriter core mcjit executionengine interpreter native)

# message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
# message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
# include_directories(${LLVM_INCLUDE_DIRS})

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config)

execute_process(
	COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs core mcjit executionengine interpreter analysis native bitwriter
	OUTPUT_VARIABLE LLVM_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(${PROJECT_SOURCE_DIR/include})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB_RECURSE LIBS    libs/*)

add_executable(luka ${SOURCES})

target_link_libraries(luka ${LLVM_LIBRARIES} ${LLVM_LIBS} ${LIBS})
