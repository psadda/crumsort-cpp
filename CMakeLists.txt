cmake_minimum_required(VERSION 3.15)
project(crumsort-cpp LANGUAGES C CXX)

add_library(crumsortcpp INTERFACE)
add_library(crumsortcpp::crumsortcpp ALIAS crumsortcpp)

target_include_directories(crumsortcpp INTERFACE src)
target_compile_features(crumsortcpp INTERFACE cxx_std_17)

option(CRUMSORT_CPP_BUILD_TESTS "Build tests" ON)
option(CRUMSORT_CPP_BUILD_BENCH "Build benchmarks" ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	if (CRUMSORT_CPP_BUILD_TESTS)
		enable_testing()
		add_subdirectory(test)
	endif()
	if (CRUMSORT_CPP_BUILD_BENCH)
		option(CRUMSORT_CPP_BENCH_WITH_X86SIMDSORT "Include x86simdsort in benchmarks" ON)
		option(CRUMSORT_CPP_BENCH_WITH_VQSORT "Include hwy::VQSort in benchmarks" ON)
		add_subdirectory(bench)
	endif()
endif()
