#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <crumsort.hpp>
#include <quadsort.hpp>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <vector>

int RandomInt() {
	// This is **not** the textbook solution to getting a random integer, but we don't really care
	// about the exact statistical distribution here; we just need numbers that aren't ordered.
	return std::rand() % 1000;
}

//////
// Nontrivial default constructor
//////

struct NonTrivialDefaultConstructor {
	int value;

	explicit NonTrivialDefaultConstructor() { value = RandomInt(); }

	bool operator<(const NonTrivialDefaultConstructor& other) const { return value < other.value; }
	bool operator==(const NonTrivialDefaultConstructor& other) const { return value == other.value; }
};

TEST_CASE("crumsort sorts types with nontrivial default constructor") {
	std::vector<NonTrivialDefaultConstructor> list;
	for (int i = 0; i < 100; ++i) list.push_back(NonTrivialDefaultConstructor());

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts types with nontrivial default constructor") {
	std::vector<NonTrivialDefaultConstructor> list;
	for (int i = 0; i < 100; ++i) list.push_back(NonTrivialDefaultConstructor());

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

//////
// No default constructor
//////

struct NoDefaultConstructor {
	int value;

	explicit NoDefaultConstructor(int value) : value(value) {}

	bool operator<(const NoDefaultConstructor& other) const { return value < other.value; }
	bool operator==(const NoDefaultConstructor& other) const { return value == other.value; }
};

TEST_CASE("crumsort sorts types without a default constructor") {
	std::vector<NoDefaultConstructor> list;
	for (int i = 0; i < 100; ++i) list.push_back(NoDefaultConstructor(RandomInt()));

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts types without a default constructor") {
	std::vector<NoDefaultConstructor> list;
	for (int i = 0; i < 100; ++i) list.push_back(NoDefaultConstructor(RandomInt()));

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

//////
// Move-only
//////

struct MoveOnly {
	int value;

	MoveOnly() = default;
	explicit MoveOnly(int value) : value(value) {}
	MoveOnly(const MoveOnly&) = delete;
	MoveOnly& operator=(const MoveOnly&) = delete;
	MoveOnly(MoveOnly&&) = default;
	MoveOnly& operator=(MoveOnly&&) = default;

	bool operator<(const MoveOnly& other) const { return value < other.value; }
	bool operator==(const MoveOnly& other) const { return value == other.value; }
};

TEST_CASE("crumsort sorts move-only types") {
	std::vector<MoveOnly> list;
	for (int i = 0; i < 100; ++i) list.push_back(MoveOnly(RandomInt()));

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts move-only types") {
	std::vector<MoveOnly> list;
	for (int i = 0; i < 100; ++i) list.push_back(MoveOnly(RandomInt()));

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}
