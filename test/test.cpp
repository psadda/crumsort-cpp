#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <crumsort.hpp>
#include <quadsort.hpp>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <vector>

int RandomInt(int max_value = 1000) {
	// This is **not** the textbook solution to getting a random integer, but we don't really care
	// about the exact statistical distribution here; we just need numbers that aren't ordered.
	return std::rand() % max_value;
}

//////
// Simple sort
//////

TEST_CASE("crumsort correctly sorts random integers") {
	std::vector<int> list;
	for (int i = 0; i < 100; ++i) list.push_back(RandomInt());

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort correctly sorts random integers") {
	std::vector<int> list;
	for (int i = 0; i < 100; ++i) list.push_back(RandomInt());

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

//////
// Custom predicate
//////

TEST_CASE("crumsort correctly sorts with a custom predicate") {
	std::vector<int> list;
	for (int i = 0; i < 100; ++i) list.push_back(RandomInt());

	scandum::crumsort(list.begin(), list.end(), std::greater<int>());

	CHECK(std::is_sorted(list.begin(), list.end(), std::greater<int>()));
}

TEST_CASE("quadsort correctly sorts with a custom predicate") {
	std::vector<int> list;
	for (int i = 0; i < 100; ++i) list.push_back(RandomInt());

	scandum::quadsort(list.begin(), list.end(), std::greater<int>());

	CHECK(std::is_sorted(list.begin(), list.end(), std::greater<int>()));
}

//////
// Stable sort
//////

struct OrderedInt {
	int value, order;
	bool operator<(const OrderedInt& other) const { return value < other.value; }
	bool operator==(const OrderedInt& other) const { return value == other.value; }
};

TEST_CASE("quadsort is stable") {
	constexpr int MAX_VALUE = 10; // Pick a small value so we're guaranteed to have equal elements

	std::vector<OrderedInt> list;
	for (int i = 0; i < 100; ++i) list.push_back({ RandomInt(MAX_VALUE), i });

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));

	CHECK(std::is_sorted(list.begin(), list.end(), [](const auto& a, const auto& b){
		return (a.value * 10000) + a.order < (b.value * 10000) + b.order;
	}));
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

static_assert (!std::is_trivially_default_constructible_v<NonTrivialDefaultConstructor>);
static_assert (std::is_default_constructible_v<NonTrivialDefaultConstructor>);

TEST_CASE("crumsort sorts types with a nontrivial default constructor") {
	std::vector<NonTrivialDefaultConstructor> list;
	for (int i = 0; i < 100; ++i) list.push_back(NonTrivialDefaultConstructor());

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts types with a nontrivial default constructor") {
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

static_assert (!std::is_default_constructible_v<NoDefaultConstructor>);

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

static_assert (!std::is_copy_constructible_v<MoveOnly>);
static_assert (!std::is_copy_assignable_v<MoveOnly>);
static_assert (std::is_move_constructible_v<MoveOnly>);
static_assert (std::is_move_assignable_v<MoveOnly>);

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

//////
// Nontrivial copy
//////

struct NoTrivialCopy {
	int value;

	NoTrivialCopy() = default;
	explicit NoTrivialCopy(int value) : value(value) {}
	NoTrivialCopy(const NoTrivialCopy& other) { value = other.value; };
	NoTrivialCopy& operator=(const NoTrivialCopy& other) { value = other.value; return *this; };

	bool operator<(const NoTrivialCopy& other) const { return value < other.value; }
	bool operator==(const NoTrivialCopy& other) const { return value == other.value; }
};

static_assert (!std::is_trivially_copy_constructible_v<NoTrivialCopy>);
static_assert (!std::is_trivially_copy_assignable_v<NoTrivialCopy>);
static_assert (std::is_copy_constructible_v<NoTrivialCopy>);
static_assert (std::is_copy_assignable_v<NoTrivialCopy>);

TEST_CASE("crumsort sorts types that are nontrivially copyable") {
	std::vector<NoTrivialCopy> list;
	for (int i = 0; i < 100; ++i) list.push_back(NoTrivialCopy(RandomInt()));

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts types that are nontrivially copyable") {
	std::vector<NoTrivialCopy> list;
	for (int i = 0; i < 100; ++i) list.push_back(NoTrivialCopy(RandomInt()));

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

//////
// Nontrivial move
//////

struct NoTrivialMove {
	int value;

	NoTrivialMove() = default;
	explicit NoTrivialMove(int value) : value(value) {}
	NoTrivialMove(const NoTrivialMove& other) = delete;
	NoTrivialMove& operator=(const NoTrivialMove& other) = delete;
	NoTrivialMove(NoTrivialMove&& other) { value = other.value; };
	NoTrivialMove& operator=(NoTrivialMove&& other) { value = other.value; return *this; };

	bool operator<(const NoTrivialMove& other) const { return value < other.value; }
	bool operator==(const NoTrivialMove& other) const { return value == other.value; }
};

static_assert (!std::is_trivially_move_constructible_v<NoTrivialMove>);
static_assert (!std::is_trivially_move_assignable_v<NoTrivialMove>);
static_assert (!std::is_copy_constructible_v<NoTrivialMove>);
static_assert (!std::is_copy_assignable_v<NoTrivialMove>);
static_assert (std::is_move_constructible_v<NoTrivialMove>);
static_assert (std::is_move_assignable_v<NoTrivialMove>);

TEST_CASE("crumsort sorts types that are nontrivially movable") {
	std::vector<NoTrivialCopy> list;
	for (int i = 0; i < 100; ++i) list.push_back(NoTrivialCopy(RandomInt()));

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts types that are nontrivially movable") {
	std::vector<NoTrivialCopy> list;
	for (int i = 0; i < 100; ++i) list.push_back(NoTrivialCopy(RandomInt()));

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

//////
// Noncontiguous memory
//////

TEST_CASE("crumsort sorts types with noncontiguous memory") {
	std::deque<int> list;
	for (int i = 0; i < 100; ++i) list.push_back(RandomInt());

	scandum::crumsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}

TEST_CASE("quadsort sorts types with noncontiguous memory") {
	std::deque<int> list;
	for (int i = 0; i < 100; ++i) list.push_back(RandomInt());

	scandum::quadsort(list.begin(), list.end());

	CHECK(std::is_sorted(list.begin(), list.end()));
}
