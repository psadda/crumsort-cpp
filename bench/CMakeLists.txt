include(FetchContent)

file(DOWNLOAD https://raw.githubusercontent.com/orlp/pdqsort/master/pdqsort.h
	${CMAKE_CURRENT_BINARY_DIR}/pdqsort.h)
file(DOWNLOAD https://raw.githubusercontent.com/mlochbaum/rhsort/master/rhsort.c
	${CMAKE_CURRENT_BINARY_DIR}/rhsort.c)
file(DOWNLOAD https://raw.githubusercontent.com/skarupke/ska_sort/master/ska_sort.hpp
	${CMAKE_CURRENT_BINARY_DIR}/ska_sort.hpp)
file(DOWNLOAD https://raw.githubusercontent.com/timsort/cpp-TimSort/master/include/gfx/timsort.hpp
	${CMAKE_CURRENT_BINARY_DIR}/timsort.hpp)

FetchContent_Declare(wolfsort GIT_REPOSITORY https://github.com/scandum/wolfsort.git)
FetchContent_MakeAvailable(wolfsort)

add_executable(crumsortcpp-benchmarks
	bench.cpp
	scandum_sorts.h
	scandum_sorts.c
	${CMAKE_CURRENT_BINARY_DIR}/rhsort.c)

target_include_directories(crumsortcpp-benchmarks PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}/_deps/wolfsort-src/src)

target_link_libraries(crumsortcpp-benchmarks crumsortcpp)

set_property(TARGET crumsortcpp-benchmarks PROPERTY COMPILE_WARNING_AS_ERROR OFF)
