include(FetchContent)
FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git)
FetchContent_MakeAvailable(doctest)

add_executable(tests test.cpp)
target_link_libraries(tests crumsortcpp doctest::doctest)

include(CTest)
include(${FETCHCONTENT_BASE_DIR}/doctest-src/scripts/cmake/doctest.cmake)
doctest_discover_tests(tests)
