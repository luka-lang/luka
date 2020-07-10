cmake_minimum_required(VERSION 3.17)

project(luka)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${PROJECT_SOURCE_DIR/include} ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB_RECURSE LIBS    libs/*)

add_executable(luka ${SOURCES})

target_link_libraries(luka ${LLVM_LIBRARIES} ${LIBS})
