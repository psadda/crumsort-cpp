include(FetchContent)

file(DOWNLOAD https://raw.githubusercontent.com/orlp/pdqsort/master/pdqsort.h
	${CMAKE_CURRENT_BINARY_DIR}/pdqsort.h)
file(DOWNLOAD https://raw.githubusercontent.com/mlochbaum/rhsort/master/rhsort.c
	${CMAKE_CURRENT_BINARY_DIR}/rhsort.c)
file(DOWNLOAD https://raw.githubusercontent.com/skarupke/ska_sort/master/ska_sort.hpp
	${CMAKE_CURRENT_BINARY_DIR}/ska_sort.hpp)
file(DOWNLOAD https://raw.githubusercontent.com/timsort/cpp-TimSort/master/include/gfx/timsort.hpp
	${CMAKE_CURRENT_BINARY_DIR}/timsort.hpp)

if(CRUMSORT_CPP_BENCH_WITH_X86SIMDSORT)
	FetchContent_Declare(simdsort GIT_REPOSITORY https://github.com/intel/x86-simd-sort GIT_TAG main GIT_SHALLOW TRUE)
	list(APPEND BENCH_REMOTE_MODULES simdsort)
	list(APPEND BENCH_INCLUDE_DIRS ${FETCHCONTENT_BASE_DIR}/simdsort-src/src)
	list(APPEND BENCH_DEFS USE_X86SIMDSORT)
endif()

if(CRUMSORT_CPP_BENCH_WITH_VQSORT)
	SET(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
	SET(HWY_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
	SET(HWY_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
	SET(HWY_SYSTEM_GTEST ON CACHE BOOL "" FORCE)
	SET(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(highway GIT_REPOSITORY https://github.com/google/highway GIT_SHALLOW TRUE)
	list(APPEND BENCH_REMOTE_MODULES highway)
	list(APPEND BENCH_LIBS hwy hwy_contrib)
	list(APPEND BENCH_DEFS USE_VQSORT)
endif()

FetchContent_Declare(wolfsort GIT_REPOSITORY https://github.com/scandum/wolfsort.git GIT_SHALLOW TRUE)
list(APPEND BENCH_REMOTE_MODULES wolfsort)
list(APPEND BENCH_INCLUDE_DIRS ${FETCHCONTENT_BASE_DIR}/wolfsort-src/src)

FetchContent_MakeAvailable(${BENCH_REMOTE_MODULES})

add_executable(benchmarks
	bench.cpp
	scandum_sorts.h
	scandum_sorts.c
	${CMAKE_CURRENT_BINARY_DIR}/rhsort.c)

target_include_directories(benchmarks PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${BENCH_INCLUDE_DIRS})
target_link_libraries(benchmarks crumsortcpp ${BENCH_LIBS})
target_compile_definitions(benchmarks PRIVATE ${BENCH_DEFS})

if(MSVC)
	target_compile_options(benchmarks PRIVATE /arch:AVX2)
else()
	target_compile_options(benchmarks PRIVATE -mavx2)
endif()

set_property(TARGET benchmarks PROPERTY COMPILE_WARNING_AS_ERROR OFF)
