cmake_minimum_required(VERSION 3.10)
project(glucose CXX)

# TODO: parallel version is not compiled

# static lib
file(GLOB_RECURSE GLUCOSE_FILES
    ${PROJECT_SOURCE_DIR}/core/*.cc
    ${PROJECT_SOURCE_DIR}/mtl/*.cc
    ${PROJECT_SOURCE_DIR}/simp/SimpSolver.cc
    ${PROJECT_SOURCE_DIR}/utils/*.cc
)
add_library(glucose_lib STATIC ${GLUCOSE_FILES})
target_include_directories(glucose_lib PUBLIC ${PROJECT_SOURCE_DIR})
set_target_properties(glucose_lib PROPERTIES OUTPUT_NAME "glucose")
set_target_properties(glucose_lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# build executable
add_executable(glucose ${PROJECT_SOURCE_DIR}/simp/Main.cc)
target_include_directories(glucose PUBLIC ${PROJECT_SOURCE_DIR})
add_dependencies(glucose glucose_lib)
target_link_libraries(glucose glucose_lib z)

# build test
add_executable(glucose_ext_test ${PROJECT_SOURCE_DIR}/example/Graph.cpp ${PROJECT_SOURCE_DIR}/example/Test.cpp)
target_include_directories(glucose_ext_test PUBLIC ${PROJECT_SOURCE_DIR})
add_dependencies(glucose_ext_test glucose_lib)
target_link_libraries(glucose_ext_test glucose_lib)

# install
install(TARGETS glucose_lib glucose
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib)

install(DIRECTORY core mtl parallel simp utils DESTINATION include/ FILES_MATCHING PATTERN "*.h")
