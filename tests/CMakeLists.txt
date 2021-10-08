include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/tests/include")
include_directories("${PROJECT_SOURCE_DIR}/tests/third_party/include")

include_directories("${PROJECT_SOURCE_DIR}")

file(GLOB sources "${PROJECT_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM sources "${PROJECT_SOURCE_DIR}/src/main.c")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.c")
list(REMOVE_ITEM tests "${PROJECT_SOURCE_DIR}/test/main.c")

# foreach(file ${tests})
#   set(name)
#   get_filename_component(name ${file} NAME_WE)
#   add_executable("${name}_tests"
#     ${sources}
#     ${file}
#     "${PROJECT_SOURCE_DIR}/tests/main.c")
#   target_link_libraries("${name}_tests" vector ${LLVM_LIBRARIES})
#   add_test(NAME ${name} COMMAND "${name}_tests")
# endforeach()
