# 'Google_test' is the subproject name
project(tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(googletest)
#include_directories(${gtest_SOURCE_DIR}/include)


# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests
add_executable(Google_Tests_run test.cpp)
find_package (glog 0.7.0 REQUIRED)
target_link_libraries(Google_Tests_run
        gtest
        gtest_main
        glog::glog
)
