#ifndef SCANDUM_QUADSORT_HPP
#define SCANDUM_QUADSORT_HPP

// quadsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

#include <algorithm>   // for std::copy and std::copy_backward
#include <cassert>
#include <cstring>     // for std::memcpy, std::memmove, std::memset
#include <optional>
#include <type_traits>
#include <vector>

// comparison functions

#define scandum_greater(less, lhs, rhs) less(get_elem<T>(rhs), get_elem<T>(lhs))
#define scandum_not_greater(less, lhs, rhs) \
	(less(get_elem<T>(lhs), get_elem<T>(rhs)) || \
	!less(get_elem<T>(rhs), get_elem<T>(lhs)))

// universal copy functions to handle both trivially and nontrivially copyable types

#define scandum_copy_range(T, output, input, length) \
	if constexpr (std::is_trivially_copyable_v<T> && \
		std::is_same_v<decltype(input), T*> && \
		std::is_same_v<decltype(output), T*>) \
	{ \
		std::memcpy(output, input, (length) * sizeof(T)); \
	} \
	else if constexpr (std::is_same_v<std::remove_reference_t<decltype(*(input))>, std::optional<T>>) \
	{ \
		auto in_begin = input; \
		auto in_end = in_begin + (length); \
		auto out = output; \
		for (auto it = in_begin; it < in_end; ++it) *out++ = **it; \
	} \
	else \
	{ \
		auto it = input; \
		std::copy(it, it + (length), output); \
	}

#define scandum_copy_overlapping_range(T, output, input, length) \
	if constexpr (std::is_trivially_copyable_v<T> && \
		std::is_same_v<decltype(input), T*> && \
		std::is_same_v<decltype(output), T*>) \
	{ \
		std::memmove(output, input, (length) * sizeof(T)); \
	} \
	else \
	{ \
		auto in_begin = input; \
		auto in_end = in_begin + (length); \
		auto out = output; \
		assert(out < in_begin || out > in_end); \
		std::copy_backward(in_begin, in_end, out); \
	}


// utilize branchless ternary operations in clang

#if !defined __clang__
#define scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp)  \
	x = scandum_not_greater(cmp, ptl, ptr);  \
	*ptd = get_elem<T>(ptl);  \
	ptl += x;  \
	ptd[x] = get_elem<T>(ptr);  \
	ptr += !x;  \
	ptd++;
#else
#define scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp)  \
	*ptd++ = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl++) : get_elem<T>(ptr++);
#endif

#if !defined __clang__
#define scandum_tail_branchless_merge(tpd, y, tpl, tpr, cmp)  \
	y = scandum_not_greater(cmp, tpl, tpr);  \
	*tpd = get_elem<T>(tpl);  \
	tpl -= !y;  \
	tpd--;  \
	tpd[y] = get_elem<T>(tpr);  \
	tpr -= y;
#else
#define scandum_tail_branchless_merge(tpd, x, tpl, tpr, cmp)  \
	*tpd-- = scandum_greater(cmp, tpl, tpr) ? get_elem<T>(tpl--) : get_elem<T>(tpr--);
#endif

// guarantee small parity merges are inlined with minimal overhead

#define scandum_parity_merge_two(array, swap, x, ptl, ptr, pts, cmp)  \
	ptl = array; ptr = array + 2; pts = swap;  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl) : get_elem<T>(ptr);  \
  \
	ptl = array + 1; ptr = array + 3; pts = swap + 3;  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_greater(cmp, ptl, ptr) ? get_elem<T>(ptl) : get_elem<T>(ptr);

#define scandum_parity_merge_four(array, swap, x, ptl, ptr, pts, cmp)  \
	ptl = array + 0; ptr = array + 4; pts = swap;  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl) : get_elem<T>(ptr);  \
  \
	ptl = array + 3; ptr = array + 7; pts = swap + 7;  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_greater(cmp, ptl, ptr) ? get_elem<T>(ptl) : get_elem<T>(ptr);


#if !defined __clang__
#define scandum_branchless_swap(pta, swap, x, cmp)  \
	x = scandum_greater(cmp, pta, pta + 1);  \
	swap = pta[!x];  \
	pta[0] = pta[x];  \
	pta[1] = swap;
#else
#define scandum_branchless_swap(pta, swap, x, cmp)  \
	x = 0;  \
	set_temp(swap, scandum_greater(cmp, pta, pta + 1) ? pta[x++] : pta[1]); \
	pta[0] = pta[x];  \
	pta[1] = get_temp<T>(swap);
#endif

#define scandum_swap_branchless(pta, swap, x, y, cmp)  \
	x = scandum_greater(cmp, pta, pta + 1);  \
	y = !x;  \
	set_temp(swap, pta[y]);  \
	pta[0] = pta[x];  \
	pta[1] = get_temp<T>(swap);


namespace scandum {

namespace detail {

template<typename T>
struct optional_value {
	std::optional<T> value;
};

template<typename T>
constexpr bool temp_value_needs_deref = false;

template<typename T>
constexpr bool temp_value_needs_deref<optional_value<T>> = true;

template<typename T>
using temp_type = typename std::conditional<std::is_default_constructible_v<T>, T, optional_value<T>>::type;

template<typename T>
class optional_it {
private:
	std::optional<T>* it;

public:
	optional_it() = default;
	explicit optional_it(std::optional<T>* it) : it(it) {}

	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using pointer = T*;
	using reference = T&;
	using difference_type = ptrdiff_t;

	optional_it& operator++() { ++it; return *this; }
	optional_it operator++(int) { auto copy = *this; ++it; return copy; }
	optional_it& operator--() { --it; return *this; }
	optional_it operator--(int) { auto copy = *this; --it; return copy; }
	optional_it operator+(ptrdiff_t distance) { return optional_it(it + distance); }
	optional_it operator-(ptrdiff_t distance) { return optional_it(it - distance); }
	ptrdiff_t operator-(const optional_it& other) { return it - other.it; }
	optional_it& operator+=(ptrdiff_t distance) { it += distance; return *this; }
	optional_it& operator-=(ptrdiff_t distance) { it -= distance; return *this; }

	const T& operator*() const { return **it; }
	std::optional<T>& operator*() { return *it; }

	friend bool operator==(const optional_it& a, const optional_it& b) { return a.it == b.it; };
	friend bool operator!=(const optional_it& a, const optional_it& b) { return a.it != b.it; };
	friend bool operator<(const optional_it& a, const optional_it& b) { return a.it < b.it; };
	friend bool operator>(const optional_it& a, const optional_it& b) { return a.it > b.it; };
	friend bool operator<=(const optional_it& a, const optional_it& b) { return a.it <= b.it; };
	friend bool operator>=(const optional_it& a, const optional_it& b) { return a.it >= b.it; };
};

template<typename T>
constexpr bool iterator_needs_double_deref = false;

template<typename T>
constexpr bool iterator_needs_double_deref<optional_it<T>> = true;

template<typename T>
class swap_space {
private:
	using storage_type = typename std::conditional<
		std::is_default_constructible_v<T>,
		std::vector<T>,               // array type for default constructible types
		std::vector<std::optional<T>> // array type for non-default constructible types
	>::type;

	storage_type storage;

public:
	explicit swap_space(size_t size) : storage(size) {}
	swap_space(const swap_space&) = delete;
	swap_space& operator=(const swap_space&) = delete;

	void emplace(size_t index, T&& value)
	{
		storage.emplace(storage.begin() + index, std::forward<T>(value));
	}

	void emplace(size_t index, const T& value)
	{
		storage.emplace(storage.begin() + index, value);
	}

	using iterator = typename std::conditional<
		std::is_default_constructible_v<T>,
		T*,            // iterator for default constructible types
		optional_it<T> // iterator for non-default constructible types
	>::type;

	constexpr auto begin()
	{
		if constexpr (std::is_default_constructible_v<T>)
		{
			return storage.data();
		}
		else
		{
			return optional_it{ storage.data() };
		}
	}

	constexpr auto end()
	{
		if constexpr (std::is_default_constructible_v<T>)
		{
			return storage.data() + storage.size();
		}
		else
		{
			return optional_it{ storage.data() + storage.size() };
		}
	}

	constexpr auto size() const { return storage.size(); }
};

template<typename T>
constexpr T& get_temp(T& value)
{
	return value;
}

template<typename T>
constexpr T& get_temp(optional_value<T>& optional)
{
	return *optional.value;
}

template<typename T>
constexpr void set_temp(T& dest, T& value)
{
	dest = value;
}

template<typename T>
constexpr T& set_temp(optional_value<T>& dest, T& value)
{
	return *dest.value = value;
}

template<typename T, typename Iterator>
constexpr T& get_elem(Iterator& value)
{
	if constexpr (iterator_needs_double_deref<Iterator>)
	{
		return **value;
	}
	else
	{
		return *value;
	}
}

template<typename T, typename Iterator>
constexpr T& get_elem(Iterator&& value)
{
	if constexpr (iterator_needs_double_deref<Iterator>)
	{
		return **value;
	}
	else
	{
		return *value;
	}
}

// the next seven functions are used for sorting 0 to 31 elements

template<typename T, typename Iterator, typename Compare>
void parity_swap_four(Iterator array, Compare cmp)
{
	temp_type<T> tmp;
	Iterator pta = array;
	size_t x;

	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta--;

	if (scandum_greater(cmp, pta, pta + 1))
	{
		set_temp(tmp, pta[0]); pta[0] = pta[1]; pta[1] = get_temp<T>(tmp); pta--;

		scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
		scandum_branchless_swap(pta, tmp, x, cmp); pta--;
		scandum_branchless_swap(pta, tmp, x, cmp);
	}
}

template<typename T, typename Iterator, typename Compare>
void parity_swap_five(Iterator array, Compare cmp)
{
	temp_type<T> tmp;
	Iterator pta = array;
	size_t x, y;

	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta -= 1;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, y, cmp); pta = array;

	if (x + y)
	{
		scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
		scandum_branchless_swap(pta, tmp, x, cmp); pta -= 1;
		scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
		scandum_branchless_swap(pta, tmp, x, cmp); pta = array;
		scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
		scandum_branchless_swap(pta, tmp, x, cmp); pta -= 1;
	}
}

template<typename T, typename Iterator, typename Compare>
void parity_swap_six(Iterator array, swap_space<T>& swap, Compare cmp)
{
	temp_type<T> tmp;
	Iterator pta = array;
	typename swap_space<T>::iterator ptl;
	typename swap_space<T>::iterator ptr;
	size_t x, y;

	scandum_branchless_swap(pta, tmp, x, cmp); pta++;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 3;
	scandum_branchless_swap(pta, tmp, x, cmp); pta--;
	scandum_branchless_swap(pta, tmp, x, cmp); pta = array;

	if (scandum_not_greater(cmp, pta + 2, pta + 3))
	{
		scandum_branchless_swap(pta, tmp, x, cmp); pta += 4;
		scandum_branchless_swap(pta, tmp, x, cmp);
		return;
	}
	x = scandum_greater(cmp, pta, pta + 1); y = !x; swap.emplace(0, pta[x]); swap.emplace(1, pta[y]); swap.emplace(2, pta[2]); pta += 4;
	x = scandum_greater(cmp, pta, pta + 1); y = !x; swap.emplace(4, pta[x]); swap.emplace(5, pta[y]); swap.emplace(3, pta[-1]);

	pta = array; ptl = swap.begin(); ptr = swap.begin() + 3;

	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);

	pta = array + 5; ptl = swap.begin() + 2; ptr = swap.begin() + 5;

	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	*pta = scandum_greater(cmp, ptl, ptr) ? get_elem<T>(ptl) : get_elem<T>(ptr);
}

template<typename T, typename Iterator, typename Compare>
void parity_swap_seven(Iterator array, swap_space<T>& swap, Compare cmp)
{
	temp_type<T> tmp;
	Iterator pta = array;
	typename swap_space<T>::iterator ptl;
	typename swap_space<T>::iterator ptr;
	size_t x, y;

	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta -= 3;
	scandum_branchless_swap(pta, tmp, y, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2; y += x;
	scandum_branchless_swap(pta, tmp, x, cmp); pta -= 1; y += x;

	if (y == 0) return;

	scandum_branchless_swap(pta, tmp, x, cmp); pta = array;

	x = scandum_greater(cmp, pta, pta + 1); swap.emplace(0, pta[x]); swap.emplace(1, pta[!x]); swap.emplace(2, pta[2]); pta += 3;
	x = scandum_greater(cmp, pta, pta + 1); swap.emplace(3, pta[x]); swap.emplace(4, pta[!x]); pta += 2;
	x = scandum_greater(cmp, pta, pta + 1); swap.emplace(5, pta[x]); swap.emplace(6, pta[!x]);

	pta = array; ptl = swap.begin(); ptr = swap.begin() + 3;

	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);

	pta = array + 6; ptl = swap.begin() + 2; ptr = swap.begin() + 6;

	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	*pta = scandum_greater(cmp, ptl, ptr) ? get_elem<T>(ptl) : get_elem<T>(ptr);
}

template<typename T, typename Iterator, typename Compare>
void tiny_sort(Iterator array, swap_space<T>& swap, size_t nmemb, Compare cmp)
{
	temp_type<T> tmp;
	size_t x;

	switch (nmemb)
	{
		case 0:
		case 1:
			return;
		case 2:
			scandum_branchless_swap(array, tmp, x, cmp);
		case 3:
			scandum_branchless_swap(array, tmp, x, cmp); array++;
			scandum_branchless_swap(array, tmp, x, cmp); array--;
			scandum_branchless_swap(array, tmp, x, cmp);
			return;
		case 4:
			parity_swap_four<T>(array, cmp);
			return;
		case 5:
			parity_swap_five<T>(array, cmp);
			return;
		case 6:
			parity_swap_six(array, swap, cmp);
			return;
		case 7:
			parity_swap_seven(array, swap, cmp);
			return;
	}
}

// left must be equal or one smaller than right

template<typename T, typename OutputIt, typename InputIt, typename Compare>
void parity_merge(OutputIt dest, InputIt from, size_t left, size_t right, Compare cmp)
{
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*dest)>> OutputT;
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*from)>> InputT;
	//static_assert (
	//	std::is_same_v<OutputT, InputT>,
	//	"output and input iterators must refer to the same type"
	//);

#if !defined __clang__
	size_t x, y;
#endif
	InputIt ptl = from;
	InputIt ptr = from + left;
	OutputIt ptd = dest;
	InputIt tpl = ptr - 1;
	InputIt tpr = tpl + right;
	OutputIt tpd = dest + left + right - 1;

	if (left < right)
	{
		*ptd++ = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl++) : get_elem<T>(ptr++);
	}
	*ptd++ = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl++) : get_elem<T>(ptr++);

#if !defined cmp && !defined __clang__ // cache limit workaround for gcc
	if (left > QUAD_CACHE)
	{
		while (--left)
		{
			*ptd++ = scandum_not_greater(cmp, *ptl, *ptr) ? *ptl++ : *ptr++;
			*tpd-- = scandum_greater(cmp, *tpl, *tpr) ? *tpl-- : *tpr--;
		}
	}
	else
#endif
	{
		while (--left)
		{
			scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp);
			scandum_tail_branchless_merge(tpd, y, tpl, tpr, cmp);
		}
	}
	*tpd = scandum_greater(cmp, tpl, tpr) ? get_elem<T>(tpl) : get_elem<T>(tpr);
}

template<typename T, typename Iterator, typename Compare>
void tail_swap(Iterator array, swap_space<T>& swap, size_t nmemb, Compare cmp)
{
	if (nmemb < 8)
	{
		tiny_sort(array, swap, nmemb, cmp);
		return;
	}
	size_t quad1, quad2, quad3, quad4, half1, half2;

	half1 = nmemb / 2;
	quad1 = half1 / 2;
	quad2 = half1 - quad1;
	half2 = nmemb - half1;
	quad3 = half2 / 2;
	quad4 = half2 - quad3;

	Iterator pta = array;

	tail_swap(pta, swap, quad1, cmp); pta += quad1;
	tail_swap(pta, swap, quad2, cmp); pta += quad2;
	tail_swap(pta, swap, quad3, cmp); pta += quad3;
	tail_swap(pta, swap, quad4, cmp);

	if (scandum_not_greater(cmp, array + quad1 - 1, array + quad1) && scandum_not_greater(cmp, array + half1 - 1, array + half1) && scandum_not_greater(cmp, pta - 1, pta))
	{
		return;
	}
	parity_merge<T>(swap.begin(), array, quad1, quad2, cmp);
	parity_merge<T>(swap.begin() + half1, array + half1, quad3, quad4, cmp);
	parity_merge<T>(array, swap.begin(), half1, half2, cmp);
}

// the next three functions create sorted blocks of 32 elements

template<typename T, typename Iterator>
void quad_reversal(Iterator pta, Iterator ptz)
{
	Iterator ptb, pty;
	temp_type<T> tmp1, tmp2;

	size_t loop = (ptz - pta) / 2;

	ptb = pta + loop;
	pty = ptz - loop;

	if (loop % 2 == 0)
	{
		set_temp(tmp2, *ptb); *ptb-- = *pty; *pty++ = get_temp<T>(tmp2); loop--;
	}

	loop /= 2;

	do
	{
		set_temp(tmp1, *pta); *pta++ = *ptz; *ptz-- = get_temp<T>(tmp1);
		set_temp(tmp2, *ptb); *ptb-- = *pty; *pty++ = get_temp<T>(tmp2);
	}
	while (loop--);
}

template<typename T, typename Iterator, typename Compare>
void quad_swap_merge(Iterator array, swap_space<T>& swap, Compare cmp)
{
	typename swap_space<T>::iterator pts;
	Iterator ptl;
	Iterator ptr;
#if !defined __clang__
	size_t x;
#endif

	scandum_parity_merge_two(array + 0, swap.begin(), x, ptl, ptr, pts, cmp);
	scandum_parity_merge_two(array + 4, swap.begin() + 4, x, ptl, ptr, pts, cmp);

	Iterator pts2;
	typename swap_space<T>::iterator ptl2;
	typename swap_space<T>::iterator ptr2;
	scandum_parity_merge_four(swap.begin(), array, x, ptl2, ptr2, pts2, cmp);
}

template<typename T, typename Iterator, typename Compare>
void tail_merge(Iterator array, swap_space<T>& swap, size_t nmemb, size_t block, Compare cmp)
{
	Iterator pta, pte;

	pte = array + nmemb;

	while (block < nmemb && block <= swap.size())
	{
		for (pta = array; pta + block < pte; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				partial_backward_merge(pta, swap, block * 2, block, cmp);

				continue;
			}
			partial_backward_merge(pta, swap, pte - pta, block, cmp);

			break;
		}
		block *= 2;
	}
}

template<typename T, typename Iterator, typename Compare>
size_t quad_swap(Iterator array, size_t nmemb, Compare cmp)
{
	temp_type<T> tmp;
	swap_space<T> swap(32);
	size_t count;
	Iterator pta, pts;
	unsigned char v1, v2, v3, v4, x;
	pta = array;

	count = nmemb / 8;

	while (count--)
	{
		v1 = scandum_greater(cmp, pta + 0, pta + 1);
		v2 = scandum_greater(cmp, pta + 2, pta + 3);
		v3 = scandum_greater(cmp, pta + 4, pta + 5);
		v4 = scandum_greater(cmp, pta + 6, pta + 7);

		switch (v1 + v2 * 2 + v3 * 4 + v4 * 8)
		{
			case 0:
				if (scandum_not_greater(cmp, pta + 1, pta + 2) && scandum_not_greater(cmp, pta + 3, pta + 4) && scandum_not_greater(cmp, pta + 5, pta + 6))
				{
					goto ordered;
				}
				quad_swap_merge(pta, swap, cmp);
				break;

			case 15:
				if (scandum_greater(cmp, pta + 1, pta + 2) && scandum_greater(cmp, pta + 3, pta + 4) && scandum_greater(cmp, pta + 5, pta + 6))
				{
					pts = pta;
					goto reversed;
				}

			default:
			not_ordered:
				x = !v1; set_temp(tmp, pta[x]); pta[0] = pta[v1]; pta[1] = get_temp(tmp); pta += 2;
				x = !v2; set_temp(tmp, pta[x]); pta[0] = pta[v2]; pta[1] = get_temp(tmp); pta += 2;
				x = !v3; set_temp(tmp, pta[x]); pta[0] = pta[v3]; pta[1] = get_temp(tmp); pta += 2;
				x = !v4; set_temp(tmp, pta[x]); pta[0] = pta[v4]; pta[1] = get_temp(tmp); pta -= 6;

				quad_swap_merge(pta, swap, cmp);
		}
		pta += 8;

		continue;

		ordered:

		pta += 8;

		if (count--)
		{
			if ((v1 = scandum_greater(cmp, pta + 0, pta + 1)) | (v2 = scandum_greater(cmp, pta + 2, pta + 3)) | (v3 = scandum_greater(cmp, pta + 4, pta + 5)) | (v4 = scandum_greater(cmp, pta + 6, pta + 7)))
			{
				if (v1 + v2 + v3 + v4 == 4 && scandum_greater(cmp, pta + 1, pta + 2) && scandum_greater(cmp, pta + 3, pta + 4) && scandum_greater(cmp, pta + 5, pta + 6))
				{
					pts = pta;
					goto reversed;
				}
				goto not_ordered;
			}
			if (scandum_not_greater(cmp, pta + 1, pta + 2) && scandum_not_greater(cmp, pta + 3, pta + 4) && scandum_not_greater(cmp, pta + 5, pta + 6))
			{
				goto ordered;
			}
			quad_swap_merge(pta, swap, cmp);
			pta += 8;
			continue;
		}
		break;

		reversed:

		pta += 8;

		if (count--)
		{
			if ((v1 = scandum_not_greater(cmp, pta + 0, pta + 1)) | (v2 = scandum_not_greater(cmp, pta + 2, pta + 3)) | (v3 = scandum_not_greater(cmp, pta + 4, pta + 5)) | (v4 = scandum_not_greater(cmp, pta + 6, pta + 7)))
			{
				// not reversed
			}
			else
			{
				if (scandum_greater(cmp, pta - 1, pta) && scandum_greater(cmp, pta + 1, pta + 2) && scandum_greater(cmp, pta + 3, pta + 4) && scandum_greater(cmp, pta + 5, pta + 6))
				{
					goto reversed;
				}
			}
			quad_reversal<T>(pts, pta - 1);

			if (v1 + v2 + v3 + v4 == 4 && scandum_not_greater(cmp, pta + 1, pta + 2) && scandum_not_greater(cmp, pta + 3, pta + 4) && scandum_not_greater(cmp, pta + 5, pta + 6))
			{
				goto ordered;
			}
			if (v1 + v2 + v3 + v4 == 0 && scandum_greater(cmp, pta + 1, pta + 2) && scandum_greater(cmp, pta + 3, pta + 4) && scandum_greater(cmp, pta + 5, pta + 6))
			{
				pts = pta;
				goto reversed;
			}

			x = !v1; set_temp(tmp, pta[v1]); pta[0] = pta[x]; pta[1] = get_temp(tmp); pta += 2;
			x = !v2; set_temp(tmp, pta[v2]); pta[0] = pta[x]; pta[1] = get_temp(tmp); pta += 2;
			x = !v3; set_temp(tmp, pta[v3]); pta[0] = pta[x]; pta[1] = get_temp(tmp); pta += 2;
			x = !v4; set_temp(tmp, pta[v4]); pta[0] = pta[x]; pta[1] = get_temp(tmp); pta -= 6;

			if (scandum_greater(cmp, pta + 1, pta + 2) || scandum_greater(cmp, pta + 3, pta + 4) || scandum_greater(cmp, pta + 5, pta + 6))
			{
				quad_swap_merge(pta, swap, cmp);
			}
			pta += 8;
			continue;
		}

		switch (nmemb % 8)
		{
			case 7: if (scandum_not_greater(cmp, pta + 5, pta + 6)) break;
			case 6: if (scandum_not_greater(cmp, pta + 4, pta + 5)) break;
			case 5: if (scandum_not_greater(cmp, pta + 3, pta + 4)) break;
			case 4: if (scandum_not_greater(cmp, pta + 2, pta + 3)) break;
			case 3: if (scandum_not_greater(cmp, pta + 1, pta + 2)) break;
			case 2: if (scandum_not_greater(cmp, pta + 0, pta + 1)) break;
			case 1: if (scandum_not_greater(cmp, pta - 1, pta + 0)) break;
			case 0:
				quad_reversal<T>(pts, pta + nmemb % 8 - 1);

				if (pts == array)
				{
					return 1;
				}
				goto reverse_end;
		}
		quad_reversal<T>(pts, pta - 1);
		break;
	}
	tail_swap(pta, swap, nmemb % 8, cmp);

	reverse_end:

	pta = array;

	for (count = nmemb / 32 ; count-- ; pta += 32)
	{
		if (scandum_not_greater(cmp, pta + 7, pta + 8) && scandum_not_greater(cmp, pta + 15, pta + 16) && scandum_not_greater(cmp, pta + 23, pta + 24))
		{
			continue;
		}
		parity_merge<T>(swap.begin(), pta, 8, 8, cmp);
		parity_merge<T>(swap.begin() + 16, pta + 16, 8, 8, cmp);
		parity_merge<T>(pta, swap.begin(), 16, 16, cmp);
	}

	if (nmemb % 32 > 8)
	{
		tail_merge(pta, swap, nmemb % 32, 8, cmp);
	}
	return 0;
}

// The next six functions are quad merge support routines

template<typename T, typename OutputIt, typename InputIt, typename Compare>
void cross_merge(OutputIt dest, InputIt from, size_t left, size_t right, Compare cmp)
{
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*dest)>> OutputT;
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*from)>> InputT;
	//static_assert (
	//	std::is_same_v<OutputT, InputT>,
	//	"output and input iterators must refer to the same type"
	//);

	size_t loop;
#if !defined __clang__
	size_t x, y;
#endif
	InputIt ptl = from;
	InputIt ptr = from + left;
	InputIt tpl = ptr - 1;
	InputIt tpr = tpl + right;

	if (left + 1 >= right && right >= left && left >= 32)
	{
		if (scandum_greater(cmp, ptl + 15, ptr) && scandum_not_greater(cmp, ptl, ptr + 15) && scandum_greater(cmp, tpl, tpr - 15) && scandum_not_greater(cmp, tpl - 15, tpr))
		{
			parity_merge<T>(dest, from, left, right, cmp);
			return;
		}
	}
	OutputIt ptd = dest;
	OutputIt tpd = dest + left + right - 1;

	while (1)
	{
		if (tpl - ptl > 8)
		{
			ptl8_ptr: if (scandum_not_greater(cmp, ptl + 7, ptr))
			{
				scandum_copy_range(T, ptd, ptl, 8); ptd += 8; ptl += 8;

				if (tpl - ptl > 8) {goto ptl8_ptr;} continue;
			}

			tpl8_tpr: if (scandum_greater(cmp, tpl - 7, tpr))
			{
				tpd -= 7; tpl -= 7; scandum_copy_range(T, tpd--, tpl--, 8);

				if (tpl - ptl > 8) {goto tpl8_tpr;} continue;
			}
		}

		if (tpr - ptr > 8)
		{
			ptl_ptr8: if (scandum_greater(cmp, ptl, ptr + 7))
			{
				scandum_copy_range(T, ptd, ptr, 8); ptd += 8; ptr += 8;

				if (tpr - ptr > 8) {goto ptl_ptr8;} continue;
			}

			tpl_tpr8: if (scandum_not_greater(cmp, tpl, tpr - 7))
			{
				tpd -= 7; tpr -= 7; scandum_copy_range(T, tpd--, tpr--, 8);

				if (tpr - ptr > 8) {goto tpl_tpr8;} continue;
			}
		}

		if (tpd - ptd < 16)
		{
			break;
		}

#if !defined cmp && !defined __clang__
		if (left > QUAD_CACHE)
		{
			loop = 8; do
			{
				*ptd++ = scandum_not_greater(cmp, *ptl, *ptr) ? *ptl++ : *ptr++;
				*tpd-- = scandum_greater(cmp, *tpl, *tpr) ? *tpl-- : *tpr--;
			}
			while (--loop);
		}
		else
#endif
		{
			loop = 8; do
			{
				scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp);
				scandum_tail_branchless_merge(tpd, y, tpl, tpr, cmp);
			}
			while (--loop);
		}
	}

	while (ptl <= tpl && ptr <= tpr)
	{
		*ptd++ = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl++) : get_elem<T>(ptr++);
	}
	while (ptl <= tpl)
	{
		*ptd++ = get_elem<T>(ptl++);
	}
	while (ptr <= tpr)
	{
		*ptd++ = get_elem<T>(ptr++);
	}
}

template<typename T, typename Iterator, typename Compare>
void quad_merge_block(Iterator array, swap_space<T>& swap, size_t block, Compare cmp)
{
	Iterator pt1, pt2, pt3;
	size_t block_x_2 = block * 2;

	pt1 = array + block;
	pt2 = pt1 + block;
	pt3 = pt2 + block;

	switch ((scandum_not_greater(cmp, pt1 - 1, pt1)) | (scandum_not_greater(cmp, pt3 - 1, pt3)) * 2)
	{
		case 0:
			cross_merge<T>(swap.begin(), array, block, block, cmp);
			cross_merge<T>(swap.begin() + block_x_2, pt2, block, block, cmp);
			break;
		case 1:
			scandum_copy_range(T, swap.begin(), array, block_x_2);
			cross_merge<T>(swap.begin() + block_x_2, pt2, block, block, cmp);
			break;
		case 2:
			cross_merge<T>(swap.begin(), array, block, block, cmp);
			scandum_copy_range(T, swap.begin() + block_x_2, pt2, block_x_2);
			break;
		case 3:
			if (scandum_not_greater(cmp, pt2 - 1, pt2))
				return;
			scandum_copy_range(T, swap.begin(), array, block_x_2 * 2);
	}
	cross_merge<T>(array, swap.begin(), block_x_2, block_x_2, cmp);
}

template<typename T, typename Iterator, typename Compare>
size_t quad_merge(Iterator array, swap_space<T>& swap, size_t nmemb, size_t block, Compare cmp)
{
	Iterator pta, pte;

	pte = array + nmemb;

	block *= 4;

	while (block <= nmemb && block <= swap.size())
	{
		pta = array;

		do
		{
			quad_merge_block(pta, swap, block / 4, cmp);

			pta += block;
		}
		while (pta + block <= pte);

		tail_merge(pta, swap, pte - pta, block / 4, cmp);

		block *= 4;
	}

	tail_merge(array, swap, nmemb, block / 4, cmp);

	return block / 2;
}

template<typename T, typename Iterator, typename Compare>
void partial_forward_merge(Iterator array, swap_space<T>& swap, size_t nmemb, size_t block, Compare cmp)
{
	typename swap_space<T>::iterator ptl, tpl;
	Iterator ptr, tpr;
	size_t x;

	if (nmemb == block)
	{
		return;
	}

	ptr = array + block;
	tpr = array + nmemb - 1;

	if (scandum_not_greater(cmp, ptr - 1, ptr))
	{
		return;
	}

	scandum_copy_range(T, swap.begin(), array, block);

	ptl = swap.begin();
	tpl = swap.begin() + block - 1;

	while (ptl < tpl - 1 && ptr < tpr - 1)
	{
		ptr2: if (scandum_greater(cmp, ptl, ptr + 1))
		{
			*array++ = *ptr++; *array++ = *ptr++;

			if (ptr < tpr - 1) {goto ptr2;} break;
		}
		if (scandum_not_greater(cmp, ptl + 1, ptr))
		{
			*array++ = get_elem<T>(ptl++); *array++ = get_elem<T>(ptl++);

			if (ptl < tpl - 1) {goto ptl2;} break;
		}

		goto cross_swap;

		ptl2: if (scandum_not_greater(cmp, ptl + 1, ptr))
		{
			*array++ = get_elem<T>(ptl++); *array++ = get_elem<T>(ptl++);

			if (ptl < tpl - 1) {goto ptl2;} break;
		}

		if (scandum_greater(cmp, ptl, ptr + 1))
		{
			*array++ = get_elem<T>(ptr++); *array++ = get_elem<T>(ptr++);

			if (ptr < tpr - 1) {goto ptr2;} break;
		}

		cross_swap:

		x = scandum_not_greater(cmp, ptl, ptr); array[x] = get_elem<T>(ptr); ptr += 1; array[!x] = get_elem<T>(ptl); ptl += 1; array += 2;
		scandum_head_branchless_merge(array, x, ptl, ptr, cmp);
	}

	while (ptl <= tpl && ptr <= tpr)
	{
		*array++ = scandum_not_greater(cmp, ptl, ptr) ? get_elem<T>(ptl++) : get_elem<T>(ptr++);
	}

	while (ptl <= tpl)
	{
		*array++ = get_elem<T>(ptl++);
	}
}

template<typename T, typename Iterator, typename Compare>
void partial_backward_merge(Iterator array, swap_space<T>& swap, size_t nmemb, size_t block, Compare cmp)
{
	Iterator tpl, tpa;
	typename swap_space<T>::iterator tpr;
	size_t right, loop, x;

	if (nmemb == block)
	{
		return;
	}

	tpl = array + block - 1;
	tpa = array + nmemb - 1;

	if (scandum_not_greater(cmp, tpl, tpl + 1))
	{
		return;
	}

	right = nmemb - block;

	if (nmemb <= swap.size() && right >= 64)
	{
		cross_merge<T>(swap.begin(), array, block, right, cmp);

		scandum_copy_range(T, array, swap.begin(), nmemb);

		return;
	}

	scandum_copy_range(T, swap.begin(), array + block, right);

	tpr = swap.begin() + right - 1;

	while (tpl > array + 16 && tpr > swap.begin() + 16)
	{
		tpl_tpr16: if (scandum_not_greater(cmp, tpl, tpr - 15))
		{
			loop = 16; do *tpa-- = get_elem<T>(tpr--); while (--loop);

			if (tpr > swap.begin() + 16) {goto tpl_tpr16;} break;
		}

		tpl16_tpr: if (scandum_greater(cmp, tpl - 15, tpr))
		{
			loop = 16; do *tpa-- = *tpl--; while (--loop);
			
			if (tpl > array + 16) {goto tpl16_tpr;} break;
		}
		loop = 8; do
		{
			if (scandum_not_greater(cmp, tpl, tpr - 1))
			{
				*tpa-- = get_elem<T>(tpr--); *tpa-- = get_elem<T>(tpr--);
			}
			else if (scandum_greater(cmp, tpl - 1, tpr))
			{
				*tpa-- = *tpl--; *tpa-- = *tpl--;
			}
			else
			{
				x = scandum_not_greater(cmp, tpl, tpr); tpa--; tpa[x] = get_elem<T>(tpr); tpr -= 1; tpa[!x] = *tpl; tpl -= 1; tpa--;
				scandum_tail_branchless_merge(tpa, x, tpl, tpr, cmp);
			}
		}
		while (--loop);
	}

	while (tpr > swap.begin() + 1 && tpl > array + 1)
	{
		tpr2: if (scandum_not_greater(cmp, tpl, tpr - 1))
		{
			*tpa-- = get_elem<T>(tpr--); *tpa-- = get_elem<T>(tpr--);
			
			if (tpr > swap.begin() + 1) {goto tpr2;} break;
		}

		if (scandum_greater(cmp, tpl - 1, tpr))
		{
			*tpa-- = get_elem<T>(tpl--); *tpa-- = get_elem<T>(tpl--);

			if (tpl > array + 1) { goto tpl2; } break;
		}
		goto cross_swap;

	tpl2: if (scandum_greater(cmp, tpl - 1, tpr))
		{
			*tpa-- = get_elem<T>(tpl--); *tpa-- = get_elem<T>(tpl--);

			if (tpl > array + 1) {goto tpl2;} break;
		}

		if (scandum_not_greater(cmp, tpl, tpr - 1))
		{
			*tpa-- = get_elem<T>(tpr--); *tpa-- = get_elem<T>(tpr--);
			
			if (tpr > swap.begin() + 1) {goto tpr2;} break;
		}
		cross_swap:

		x = scandum_not_greater(cmp, tpl, tpr); tpa--; tpa[x] = get_elem<T>(tpr); tpr -= 1; tpa[!x] = get_elem<T>(tpl); tpl -= 1; tpa--;
		scandum_tail_branchless_merge(tpa, x, tpl, tpr, cmp);
	}

	while (tpr >= swap.begin() && tpl >= array)
	{
		*tpa-- = scandum_greater(cmp, tpl, tpr) ? get_elem<T>(tpl--) : get_elem<T>(tpr--);
	}

	while (tpr >= swap.begin())
	{
		*tpa-- = get_elem<T>(tpr--);
	}
}

// the next four functions provide in-place rotate merge support

template<typename T, typename Iterator>
void trinity_rotation(Iterator array, swap_space<T>& swap, size_t nmemb, size_t left)
{
	temp_type<T> temp;
	size_t bridge, right = nmemb - left;

	size_t swap_size = swap.size() < 65536 ? swap.size() : 65536;

	if (left < right)
	{
		if (left <= swap_size)
		{
			scandum_copy_range(T, swap.begin(), array, left);
			scandum_copy_overlapping_range(T, array, array + left, right);
			scandum_copy_range(T, array + right, swap.begin(), left);
		}
		else
		{
			Iterator pta, ptb, ptc, ptd;

			pta = array;
			ptb = pta + left;

			bridge = right - left;

			if (bridge <= swap_size && bridge > 3)
			{
				ptc = pta + right;
				ptd = ptc + left;

				scandum_copy_range(T, swap.begin(), ptb, bridge);

				while (left--)
				{
					*--ptc = *--ptd; *ptd = *--ptb;
				}
				scandum_copy_range(T, pta, swap.begin(), bridge);
			}
			else
			{
				ptc = ptb;
				ptd = ptc + right;

				bridge = left / 2;

				while (bridge--)
				{
					set_temp(temp, *--ptb); *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = get_temp(temp);
				}

				bridge = (ptd - ptc) / 2;

				while (bridge--)
				{
					set_temp(temp, *ptc); *ptc++ = *--ptd; *ptd = *pta; *pta++ = get_temp(temp);
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					set_temp(temp, *pta); *pta++ = *--ptd; *ptd = get_temp(temp);
				}
			}
		}
	}
	else if (right < left)
	{
		if (right <= swap_size)
		{
			scandum_copy_range(T, swap.begin(), array + left, right);
			scandum_copy_overlapping_range(T, array + right, array, left);
			scandum_copy_range(T, array, swap.begin(), right);
		}
		else
		{
			Iterator pta, ptb, ptc, ptd;

			pta = array;
			ptb = pta + left;

			bridge = left - right;

			if (bridge <= swap_size && bridge > 3)
			{
				ptc = pta + right;
				ptd = ptc + left;

				scandum_copy_range(T, swap.begin(), ptc, bridge);

				while (right--)
				{
					*ptc++ = *pta; *pta++ = *ptb++;
				}
				scandum_copy_range(T, ptd - bridge, swap.begin(), bridge);
			}
			else
			{
				ptc = ptb;
				ptd = ptc + right;

				bridge = right / 2;

				while (bridge--)
				{
					set_temp(temp, *--ptb); *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = get_temp(temp);
				}

				bridge = (ptb - pta) / 2;

				while (bridge--)
				{
					set_temp(temp, *--ptb); *ptb = *pta; *pta++ = *--ptd; *ptd = get_temp(temp);
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					set_temp(temp, *pta); *pta++ = *--ptd; *ptd = get_temp(temp);
				}
			}
		}
	}
	else
	{
		Iterator pta, ptb;

		pta = array;
		ptb = pta + left;

		while (left--)
		{
			set_temp(temp, *pta); *pta++ = *ptb; *ptb++ = get_temp(temp);
		}
	}
}

template<typename T, typename Iterator, typename Compare>
size_t monobound_binary_first(Iterator array, Iterator value, size_t top, Compare cmp)
{
	size_t mid;
	Iterator end = array + top;

	while (top > 1)
	{
		mid = top / 2;

		if (scandum_not_greater(cmp, value, end - mid))
		{
			end -= mid;
		}
		top -= mid;
	}

	if (scandum_not_greater(cmp, value, end - 1))
	{
		end--;
	}
	return (end - array);
}

template<typename T, typename Iterator, typename Compare>
void rotate_merge_block(Iterator array, swap_space<T>& swap, size_t lblock, size_t right, Compare cmp)
{
	size_t left, rblock, unbalanced;

	if (scandum_not_greater(cmp, array + lblock - 1, array + lblock))
	{
		return;
	}

	rblock = lblock / 2;
	lblock -= rblock;

	left = monobound_binary_first<T>(array + lblock + rblock, array + lblock, right, cmp);

	right -= left;

	// [ lblock ] [ rblock ] [ left ] [ right ]

	if (left)
	{
		if (lblock + left <= swap.size())
		{
			scandum_copy_range(T, swap.begin(), array, lblock);
			scandum_copy_range(T, swap.begin() + lblock, array + lblock + rblock, left);
			scandum_copy_overlapping_range(T, array + lblock + left, array + lblock, rblock);

			cross_merge<T>(array, swap.begin(), lblock, left, cmp);
		}
		else
		{
			trinity_rotation(array + lblock, swap, rblock + left, rblock);

			unbalanced = (left * 2 < lblock) | (lblock * 2 < left);

			if (unbalanced && left <= swap.size())
			{
				partial_backward_merge(array, swap, lblock + left, lblock, cmp);
			}
			else if (unbalanced && lblock <= swap.size())
			{
				partial_forward_merge(array, swap, lblock + left, lblock, cmp);
			}
			else
			{
				rotate_merge_block(array, swap, lblock, left, cmp);
			}
		}
	}

	if (right)
	{
		unbalanced = (right * 2 < rblock) | (rblock * 2 < right);

		if ((unbalanced && right <= swap.size()) || right + rblock <= swap.size())
		{
			partial_backward_merge(array + lblock + left, swap, rblock + right, rblock, cmp);
		}
		else if (unbalanced && rblock <= swap.size())
		{
			partial_forward_merge(array + lblock + left, swap, rblock + right, rblock, cmp);
		}
		else
		{
			rotate_merge_block(array + lblock + left, swap, rblock, right, cmp);
		}
	}
}

template<typename T, typename Iterator, typename Compare>
void rotate_merge(Iterator array, swap_space<T>& swap, size_t nmemb, size_t block, Compare cmp)
{
	Iterator pta, pte;

	pte = array + nmemb;

	if (nmemb <= block * 2 && nmemb - block <= swap.size())
	{
		partial_backward_merge(array, swap, nmemb, block, cmp);

		return;
	}

	while (block < nmemb)
	{
		for (pta = array ; pta + block < pte ; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				rotate_merge_block(pta, swap, block, block, cmp);

				continue;
			}
			rotate_merge_block(pta, swap, block, pte - pta - block, cmp);

			break;
		}
		block *= 2;
	}
}

template<typename T, typename Iterator, typename Compare>
void quadsort_swap(Iterator array, swap_space<T>& swap, size_t nmemb, Compare cmp)
{
	Iterator pta = array;

	if (nmemb <= 96)
	{
		tail_swap(pta, swap, nmemb, cmp);
	}
	else if (quad_swap<T>(pta, nmemb, cmp) == 0)
	{
		size_t block = quad_merge(pta, swap, nmemb, 32, cmp);

		rotate_merge(pta, swap, nmemb, block, cmp);
	}
}

} // namespace scandum::detail

template<typename Iterator, typename Compare>
void quadsort(Iterator begin, Iterator end, Compare cmp)
{
	typedef std::remove_reference_t<decltype(*begin)> T;

	size_t nmemb = std::distance(begin, end);
	Iterator pta = begin;

	if (nmemb < 32)
	{
		detail::swap_space<T> swap(nmemb);
		detail::tail_swap(pta, swap, nmemb, cmp);
	}
	else if (detail::quad_swap<T>(pta, nmemb, cmp) == 0)
	{
		size_t block, swap_size = nmemb;

		if (nmemb > 4194304) for (swap_size = 4194304 ; swap_size * 8 <= nmemb ; swap_size *= 4) {}

		detail::swap_space<T> swap(swap_size);

		// TODO: consider adding this path back as an optimization
		//if (swap == NULL)
		//{
		//	T stack[512];

		//	block = detail::quad_merge(pta, stack, 512, nmemb, 32, cmp);

		//	detail::rotate_merge(pta, stack, 512, nmemb, block, cmp);

		//	return;
		//}
		block = detail::quad_merge(pta, swap, nmemb, 32, cmp);

		detail::rotate_merge(pta, swap, nmemb, block, cmp);
	}
}

template<typename Iterator>
void quadsort(Iterator begin, Iterator end)
{
	typedef std::remove_reference_t<decltype(*begin)> T;
	return quadsort(begin, end, std::less<T>());
}

} // namespace scandum

#undef scandum_greater
#undef scandum_not_greater
#undef scandum_head_branchless_merge
#undef scandum_tail_branchless_merge
#undef scandum_parity_merge_two
#undef scandum_parity_merge_four
#undef scandum_branchless_swap
#undef scandum_swap_branchless

#endif
