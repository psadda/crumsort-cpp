include(FetchContent)

FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git)
FetchContent_MakeAvailable(doctest)

add_executable(crumsortcpp-tests test.cpp)
target_link_libraries(crumsortcpp-tests crumsortcpp doctest::doctest)
