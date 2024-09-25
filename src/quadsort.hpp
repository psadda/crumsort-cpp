#ifndef SCANDUM_QUADSORT_HPP
#define SCANDUM_QUADSORT_HPP

// quadsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

#include <algorithm>   // for std::copy and std::copy_backward
#include <cassert>
#include <optional>
#include <type_traits>
#include <vector>

// comparison functions

#define scandum_greater(less, lhs, rhs) less(rhs, lhs)
#define scandum_not_greater(less, lhs, rhs) (less(lhs, rhs) || !less(rhs, lhs))

// universal copy functions to handle both trivially and nontrivially copyable types

#define scandum_copy_range(T, output, input, length) \
	{ \
		auto it = input; \
		std::move(it, it + (length), output); \
	}

#define scandum_copy_overlapping_range(T, output, input, length) \
	{ \
		auto len = length; \
		auto in_begin = input; \
		auto in_end = in_begin + len; \
		auto out = output; \
		if (in_begin == out || in_begin == in_end) { \
			/* do nothing */ \
		} else if (out > in_begin && out < in_end) { \
			std::move_backward(in_begin, in_end, out + len); \
		} else { \
			std::move(in_begin, in_end, out); \
		} \
	}

#define scandum_move(x) std::move(x)

// utilize branchless ternary operations in clang

#if !defined __clang__
#define scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp)  \
	x = scandum_not_greater(cmp, *ptl, *ptr);  \
	*ptd = scandum_move(*ptl);  \
	ptl += x;  \
	ptd[x] = scandum_move(*ptr);  \
	ptr += !x;  \
	ptd++;
#else
#define scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp)  \
	*ptd++ = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? (T&)*ptl++ : (T&)*ptr++);
#endif

#if !defined __clang__
#define scandum_tail_branchless_merge(tpd, y, tpl, tpr, cmp)  \
	y = scandum_not_greater(cmp, *tpl, *tpr);  \
	*tpd = scandum_move(*tpl);  \
	tpl -= !y;  \
	tpd--;  \
	tpd[y] = scandum_move(*tpr);  \
	tpr -= y;
#else
#define scandum_tail_branchless_merge(tpd, x, tpl, tpr, cmp)  \
	*tpd-- = scandum_move(scandum_greater(cmp, *tpl, *tpr) ? (T&)*tpl-- : (T&)*tpr--);
#endif

// guarantee small parity merges are inlined with minimal overhead

#define scandum_parity_merge_two(array, swap, x, ptl, ptr, pts, cmp)  \
	ptl = array; ptr = array + 2; pts = swap;  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? (T&)*ptl : (T&)*ptr);  \
	ptl = array + 1; ptr = array + 3; pts = swap + 3;  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_move(scandum_greater(cmp, *ptl, *ptr) ? (T&)*ptl : (T&)*ptr);

#define scandum_parity_merge_four(array, swap, x, ptl, ptr, pts, cmp)  \
	ptl = array + 0; ptr = array + 4; pts = swap;  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? (T&)*ptl : (T&)*ptr);  \
	ptl = array + 3; ptr = array + 7; pts = swap + 7;  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = scandum_move(scandum_greater(cmp, *ptl, *ptr) ? (T&)*ptl : (T&)*ptr);

#if !defined __clang__
#define scandum_branchless_swap(pta, swap, x, cmp)  \
	x = scandum_greater(cmp, *pta, *(pta + 1));  \
	swap = scandum_move(pta[!x]);  \
	pta[0] = scandum_move(pta[x]);  \
	pta[1] = scandum_move(swap);
#else
#define scandum_branchless_swap(pta, swap, x, cmp)  \
	x = 0;  \
	swap = scandum_move(scandum_greater(cmp, *pta, *(pta + 1)) ? pta[x++] : pta[1]);  \
	pta[0] = scandum_move(pta[x]);  \
	pta[1] = scandum_move(swap);
#endif

#define scandum_swap_branchless(pta, swap, x, y, cmp)  \
	x = scandum_greater(cmp, *pta, *(pta + 1));  \
	y = !x;  \
	swap = scandum_move(pta[y]);  \
	pta[0] = scandum_move(pta[x]);  \
	pta[1] = scandum_move(swap);


namespace scandum {

namespace detail {

template<typename T>
struct deferred_construct : public std::optional<T> {
	deferred_construct() {}
	deferred_construct(const T& value) : std::optional<T>(value) {}
	deferred_construct(T&& value) : std::optional<T>(std::forward<T>(value)) {}
	constexpr operator T&() { return **this; }
	constexpr operator const T&() const { return **this; }
};

template<typename T>
struct swap_vector : public std::vector<T> {
	using base_type = std::vector<T>;
	using iterator = T*;
	explicit swap_vector(size_t n) : base_type(n) {}
	constexpr T* begin() { return base_type::data(); }
	constexpr T* end() { return base_type::data() + base_type::size(); }
};

template<typename T>
using swap_space = std::conditional_t<
	std::is_default_constructible_v<T>,
	swap_vector<T>,
	swap_vector<deferred_construct<T>>
>;

template<typename T>
using temp_var = std::conditional_t<
	std::is_default_constructible_v<T>,
	T,
	deferred_construct<T>
>;

// the next seven functions are used for sorting 0 to 31 elements

template<typename T, typename Iterator, typename Compare>
void parity_swap_four(Iterator array, Compare cmp)
{
	temp_var<T> tmp;
	Iterator pta = array;
	size_t x;

	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta--;

	if (scandum_greater(cmp, *pta, *(pta + 1)))
	{
		tmp = scandum_move(pta[0]); pta[0] = scandum_move(pta[1]); pta[1] = scandum_move(tmp); pta--;

		scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
		scandum_branchless_swap(pta, tmp, x, cmp); pta--;
		scandum_branchless_swap(pta, tmp, x, cmp);
	}
}

template<typename T, typename Iterator, typename Compare>
void parity_swap_five(Iterator array, Compare cmp)
{
	temp_var<T> tmp;
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
	temp_var<T> tmp;
	Iterator pta = array;
	typename swap_space<T>::iterator ptl;
	typename swap_space<T>::iterator ptr;
	size_t x, y;

	scandum_branchless_swap(pta, tmp, x, cmp); pta++;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 3;
	scandum_branchless_swap(pta, tmp, x, cmp); pta--;
	scandum_branchless_swap(pta, tmp, x, cmp); pta = array;

	if (scandum_not_greater(cmp, *(pta + 2), *(pta + 3)))
	{
		scandum_branchless_swap(pta, tmp, x, cmp); pta += 4;
		scandum_branchless_swap(pta, tmp, x, cmp);
		return;
	}
	x = scandum_greater(cmp, *pta, *(pta + 1));
	y = !x;
	swap[0] = scandum_move(pta[x]);
	swap[1] = scandum_move(pta[y]);
	swap[2] = scandum_move(pta[2]);
	pta += 4;
	x = scandum_greater(cmp, *pta, *(pta + 1));
	y = !x;
	swap[4] = scandum_move(pta[x]);
	swap[5] = scandum_move(pta[y]);
	swap[3] = scandum_move(pta[-1]);

	pta = array; ptl = swap.begin(); ptr = swap.begin() + 3;

	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);

	pta = array + 5; ptl = swap.begin() + 2; ptr = swap.begin() + 5;

	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	*pta = scandum_move(scandum_greater(cmp, *ptl, *ptr) ? *ptl : *ptr);
}

template<typename T, typename Iterator, typename Compare>
void parity_swap_seven(Iterator array, swap_space<T>& swap, Compare cmp)
{
	temp_var<T> tmp;
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

	x = scandum_greater(cmp, *pta, *(pta + 1));
	swap[0] = scandum_move(pta[x]);
	swap[1] = scandum_move(pta[!x]);
	swap[2] = scandum_move(pta[2]);
	pta += 3;
	x = scandum_greater(cmp, *pta, *(pta + 1));
	swap[3] = scandum_move(pta[x]);
	swap[4] = scandum_move(pta[!x]);
	pta += 2;
	x = scandum_greater(cmp, *pta, *(pta + 1));
	swap[5] = scandum_move(pta[x]);
	swap[6] = scandum_move(pta[!x]);

	pta = array; ptl = swap.begin(); ptr = swap.begin() + 3;

	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);

	pta = array + 6; ptl = swap.begin() + 2; ptr = swap.begin() + 6;

	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	*pta = scandum_move(scandum_greater(cmp, *ptl, *ptr) ? *ptl : *ptr);
}

template<typename T, typename Iterator, typename Compare>
void tiny_sort(Iterator array, swap_space<T>& swap, size_t nmemb, Compare cmp)
{
	temp_var<T> tmp;
	size_t x;

	switch (nmemb)
	{
		case 0:
		case 1:
			return;
		case 2:
			scandum_branchless_swap(array, tmp, x, cmp);
			return;
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
			parity_swap_six<T>(array, swap, cmp);
			return;
		case 7:
			parity_swap_seven<T>(array, swap, cmp);
			return;
	}
}

// left must be equal or one smaller than right

template<typename T, typename OutputIt, typename InputIt, typename Compare>
void parity_merge(OutputIt dest, InputIt from, size_t left, size_t right, Compare cmp)
{
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
		*ptd++ = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? *ptl++ : *ptr++);
	}
	*ptd++ = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? *ptl++ : *ptr++);

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
	*tpd = scandum_move(scandum_greater(cmp, *tpl, *tpr) ? *tpl : *tpr);
}

template<typename T, typename Iterator, typename Compare>
void tail_swap(Iterator array, swap_space<T>& swap, size_t nmemb, Compare cmp)
{
	if (nmemb < 8)
	{
		tiny_sort<T>(array, swap, nmemb, cmp);
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

	tail_swap<T>(pta, swap, quad1, cmp); pta += quad1;
	tail_swap<T>(pta, swap, quad2, cmp); pta += quad2;
	tail_swap<T>(pta, swap, quad3, cmp); pta += quad3;
	tail_swap<T>(pta, swap, quad4, cmp);

	if (scandum_not_greater(cmp, *(array + quad1 - 1), *(array + quad1)) && scandum_not_greater(cmp, *(array + half1 - 1), *(array + half1)) && scandum_not_greater(cmp, *(pta - 1), *pta))
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
	temp_var<T> tmp1, tmp2;

	size_t loop = (ptz - pta) / 2;

	ptb = pta + loop;
	pty = ptz - loop;

	if (loop % 2 == 0)
	{
		tmp2 = scandum_move(*ptb); *ptb-- = scandum_move(*pty); *pty++ = scandum_move(tmp2); loop--;
	}

	loop /= 2;

	do
	{
		tmp1 = scandum_move(*pta); *pta++ = scandum_move(*ptz); *ptz-- = scandum_move(tmp1);
		tmp2 = scandum_move(*ptb); *ptb-- = scandum_move(*pty); *pty++ = scandum_move(tmp2);
	} while (loop--);
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
	scandum_parity_merge_two(array + 0, swap.begin() + 0, x, ptl, ptr, pts, cmp);
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
		for (pta = array ; pta + block < pte ; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				partial_backward_merge<T>(pta, swap, block * 2, block, cmp);

				continue;
			}
			partial_backward_merge<T>(pta, swap, pte - pta, block, cmp);

			break;
		}
		block *= 2;
	}
}

template<typename T, typename Iterator, typename Compare>
size_t quad_swap(Iterator array, size_t nmemb, Compare cmp)
{
	temp_var<T> tmp;
	size_t count;
	Iterator pta, pts;
	unsigned char v1, v2, v3, v4, x;
	pta = array;

	swap_space<T> swap(32);

	count = nmemb / 8;

	while (count--)
	{
		v1 = scandum_greater(cmp, *(pta + 0), *(pta + 1));
		v2 = scandum_greater(cmp, *(pta + 2), *(pta + 3));
		v3 = scandum_greater(cmp, *(pta + 4), *(pta + 5));
		v4 = scandum_greater(cmp, *(pta + 6), *(pta + 7));

		switch (v1 + v2 * 2 + v3 * 4 + v4 * 8)
		{
			case 0:
				if (scandum_not_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_not_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_not_greater(cmp, *(pta + 5), *(pta + 6)))
				{
					goto ordered;
				}
				quad_swap_merge<T>(pta, swap, cmp);
				break;

			case 15:
				if (scandum_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_greater(cmp, *(pta + 5), *(pta + 6)))
				{
					pts = pta;
					goto reversed;
				}

			default:
			not_ordered:
				x = !v1; tmp = scandum_move(pta[x]); pta[0] = scandum_move(pta[v1]); pta[1] = scandum_move(tmp); pta += 2;
				x = !v2; tmp = scandum_move(pta[x]); pta[0] = scandum_move(pta[v2]); pta[1] = scandum_move(tmp); pta += 2;
				x = !v3; tmp = scandum_move(pta[x]); pta[0] = scandum_move(pta[v3]); pta[1] = scandum_move(tmp); pta += 2;
				x = !v4; tmp = scandum_move(pta[x]); pta[0] = scandum_move(pta[v4]); pta[1] = scandum_move(tmp); pta -= 6;

				quad_swap_merge<T>(pta, swap, cmp);
			}
			pta += 8;

			continue;

		ordered:

			pta += 8;

			if (count--)
			{
				if ((v1 = scandum_greater(cmp, *(pta + 0), *(pta + 1))) | (v2 = scandum_greater(cmp, *(pta + 2), *(pta + 3))) | (v3 = scandum_greater(cmp, *(pta + 4), *(pta + 5))) | (v4 = scandum_greater(cmp, *(pta + 6), *(pta + 7))))
				{
					if (v1 + v2 + v3 + v4 == 4 && scandum_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_greater(cmp, *(pta + 5), *(pta + 6)))
					{
						pts = pta;
						goto reversed;
					}
					goto not_ordered;
				}
				if (scandum_not_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_not_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_not_greater(cmp, *(pta + 5), *(pta + 6)))
				{
					goto ordered;
				}
				quad_swap_merge<T>(pta, swap, cmp);
				pta += 8;
				continue;
			}
			break;

		reversed:

			pta += 8;

			if (count--)
			{
				if ((v1 = scandum_not_greater(cmp, *(pta + 0), *(pta + 1))) | (v2 = scandum_not_greater(cmp, *(pta + 2), *(pta + 3))) | (v3 = scandum_not_greater(cmp, *(pta + 4), *(pta + 5))) | (v4 = scandum_not_greater(cmp, *(pta + 6), *(pta + 7))))
				{
					// not reversed
				}
				else
				{
					if (scandum_greater(cmp, *(pta - 1), *pta) && scandum_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_greater(cmp, *(pta + 5), *(pta + 6)))
					{
						goto reversed;
					}
				}
				quad_reversal<T>(pts, pta - 1);

				if (v1 + v2 + v3 + v4 == 4 && scandum_not_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_not_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_not_greater(cmp, *(pta + 5), *(pta + 6)))
				{
					goto ordered;
				}
				if (v1 + v2 + v3 + v4 == 0 && scandum_greater(cmp, *(pta + 1), *(pta + 2)) && scandum_greater(cmp, *(pta + 3), *(pta + 4)) && scandum_greater(cmp, *(pta + 5), *(pta + 6)))
				{
					pts = pta;
					goto reversed;
				}

				x = !v1; tmp = scandum_move(pta[v1]); pta[0] = scandum_move(pta[x]); pta[1] = scandum_move(tmp); pta += 2;
				x = !v2; tmp = scandum_move(pta[v2]); pta[0] = scandum_move(pta[x]); pta[1] = scandum_move(tmp); pta += 2;
				x = !v3; tmp = scandum_move(pta[v3]); pta[0] = scandum_move(pta[x]); pta[1] = scandum_move(tmp); pta += 2;
				x = !v4; tmp = scandum_move(pta[v4]); pta[0] = scandum_move(pta[x]); pta[1] = scandum_move(tmp); pta -= 6;

				if (scandum_greater(cmp, *(pta + 1), *(pta + 2)) || scandum_greater(cmp, *(pta + 3), *(pta + 4)) || scandum_greater(cmp, *(pta + 5), *(pta + 6)))
				{
					quad_swap_merge<T>(pta, swap, cmp);
				}
				pta += 8;
				continue;
		}

		switch (nmemb % 8)
		{
			case 7: if (scandum_not_greater(cmp, *(pta + 5), *(pta + 6))) break;
			case 6: if (scandum_not_greater(cmp, *(pta + 4), *(pta + 5))) break;
			case 5: if (scandum_not_greater(cmp, *(pta + 3), *(pta + 4))) break;
			case 4: if (scandum_not_greater(cmp, *(pta + 2), *(pta + 3))) break;
			case 3: if (scandum_not_greater(cmp, *(pta + 1), *(pta + 2))) break;
			case 2: if (scandum_not_greater(cmp, *(pta + 0), *(pta + 1))) break;
			case 1: if (scandum_not_greater(cmp, *(pta - 1), *(pta + 0))) break;
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
	tail_swap<T>(pta, swap, nmemb % 8, cmp);

reverse_end:

	pta = array;

	for (count = nmemb / 32 ; count-- ; pta += 32)
	{
		if (scandum_not_greater(cmp, *(pta + 7), *(pta + 8)) && scandum_not_greater(cmp, *(pta + 15), *(pta + 16)) && scandum_not_greater(cmp, *(pta + 23), *(pta + 24)))
		{
			continue;
		}
		parity_merge<T>(swap.begin(), pta, 8, 8, cmp);
		parity_merge<T>(swap.begin() + 16, pta + 16, 8, 8, cmp);
		parity_merge<T>(pta, swap.begin(), 16, 16, cmp);
	}

	if (nmemb % 32 > 8)
	{
		tail_merge<T>(pta, swap, nmemb % 32, 8, cmp);
	}
	return 0;
}

// The next six functions are quad merge support routines

template<typename T, typename OutputIt, typename InputIt, typename Compare>
void cross_merge(OutputIt dest, InputIt from, size_t left, size_t right, Compare cmp)
{
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
		if (scandum_greater(cmp, *(ptl + 15), *ptr) && scandum_not_greater(cmp, *ptl, *(ptr + 15)) && scandum_greater(cmp, *tpl, *(tpr - 15)) && scandum_not_greater(cmp, *(tpl - 15), *tpr))
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
		ptl8_ptr:
			if (scandum_not_greater(cmp, *(ptl + 7), *ptr))
			{
				scandum_copy_range(T, ptd, ptl, 8); ptd += 8; ptl += 8;

				if (tpl - ptl > 8) { goto ptl8_ptr; } continue;
			}

		tpl8_tpr:
			if (scandum_greater(cmp, *(tpl - 7), *tpr))
			{
				tpd -= 7; tpl -= 7; scandum_copy_range(T, tpd--, tpl--, 8);

				if (tpl - ptl > 8) { goto tpl8_tpr; } continue;
			}
		}

		if (tpr - ptr > 8)
		{
		ptl_ptr8:
			if (scandum_greater(cmp, *ptl, *(ptr + 7)))
			{
				scandum_copy_range(T, ptd, ptr, 8); ptd += 8; ptr += 8;

				if (tpr - ptr > 8) { goto ptl_ptr8; } continue;
			}

		tpl_tpr8:
			if (scandum_not_greater(cmp, *tpl, *(tpr - 7)))
			{
				tpd -= 7; tpr -= 7; scandum_copy_range(T, tpd--, tpr--, 8);

				if (tpr - ptr > 8) { goto tpl_tpr8; } continue;
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
			} while (--loop);
		}
		else
#endif
		{
			loop = 8; do
			{
				scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp);
				scandum_tail_branchless_merge(tpd, y, tpl, tpr, cmp);
			} while (--loop);
		}
	}

	while (ptl <= tpl && ptr <= tpr)
	{
		*ptd++ = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? *ptl++ : *ptr++);
	}
	while (ptl <= tpl)
	{
		*ptd++ = scandum_move(*ptl++);
	}
	while (ptr <= tpr)
	{
		*ptd++ = scandum_move(*ptr++);
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

	switch ((scandum_not_greater(cmp, *(pt1 - 1), *pt1)) | (scandum_not_greater(cmp, *(pt3 - 1), *pt3)) * 2)
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
			if (scandum_not_greater(cmp, *(pt2 - 1), *pt2))
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
			quad_merge_block<T>(pta, swap, block / 4, cmp);

			pta += block;
		} while (pta + block <= pte);

		tail_merge<T>(pta, swap, pte - pta, block / 4, cmp);

		block *= 4;
	}

	tail_merge<T>(array, swap, nmemb, block / 4, cmp);

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

	if (scandum_not_greater(cmp, *(ptr - 1), *ptr))
	{
		return;
	}

	scandum_copy_range(T, swap.begin(), array, block);

	ptl = swap.begin();
	tpl = swap.begin() + block - 1;

	while (ptl < tpl - 1 && ptr < tpr - 1)
	{
	ptr2:
		if (scandum_greater(cmp, *ptl, *(ptr + 1)))
		{
			*array++ = scandum_move(*ptr++); *array++ = scandum_move(*ptr++);

			if (ptr < tpr - 1) { goto ptr2; } break;
		}
		if (scandum_not_greater(cmp, *(ptl + 1), *ptr))
		{
			*array++ = scandum_move(*ptl++); *array++ = scandum_move(*ptl++);

			if (ptl < tpl - 1) { goto ptl2; } break;
		}

		goto cross_swap;

	ptl2:
		if (scandum_not_greater(cmp, *(ptl + 1), *ptr))
		{
			*array++ = scandum_move(*ptl++); *array++ = scandum_move(*ptl++);

			if (ptl < tpl - 1) { goto ptl2; } break;
		}

		if (scandum_greater(cmp, *ptl, *(ptr + 1)))
		{
			*array++ = scandum_move(*ptr++); *array++ = scandum_move(*ptr++);

			if (ptr < tpr - 1) { goto ptr2; } break;
		}

	cross_swap:

		x = scandum_not_greater(cmp, *ptl, *ptr);
		array[x] = scandum_move(*ptr);
		ptr += 1;
		array[!x] = scandum_move(*ptl);
		ptl += 1;
		array += 2;
		scandum_head_branchless_merge(array, x, ptl, ptr, cmp);
	}

	while (ptl <= tpl && ptr <= tpr)
	{
		*array++ = scandum_move(scandum_not_greater(cmp, *ptl, *ptr) ? (T&)*ptl++ : (T&)*ptr++);
	}

	while (ptl <= tpl)
	{
		*array++ = scandum_move(*ptl++);
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

	if (scandum_not_greater(cmp, *tpl, *(tpl + 1)))
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
	tpl_tpr16:
		if (scandum_not_greater(cmp, *tpl, *(tpr - 15)))
		{
			loop = 16; do *tpa-- = scandum_move(*tpr--); while (--loop);

			if (tpr > swap.begin() + 16) { goto tpl_tpr16; } break;
		}

	tpl16_tpr:
		if (scandum_greater(cmp, *(tpl - 15), *tpr))
		{
			loop = 16; do *tpa-- = scandum_move(*tpl--); while (--loop);

			if (tpl > array + 16) { goto tpl16_tpr; } break;
		}
	loop = 8;
		do
		{
			if (scandum_not_greater(cmp, *tpl, *(tpr - 1)))
			{
				*tpa-- = scandum_move(*tpr--); *tpa-- = scandum_move(*tpr--);
			}
			else if (scandum_greater(cmp, *(tpl - 1), *tpr))
			{
				*tpa-- = scandum_move(*tpl--); *tpa-- = scandum_move(*tpl--);
			}
			else
			{
				x = scandum_not_greater(cmp, *tpl, *tpr);
				tpa--;
				tpa[x] = scandum_move(*tpr);
				tpr -= 1;
				tpa[!x] = scandum_move(*tpl);
				tpl -= 1;
				tpa--;
				scandum_tail_branchless_merge(tpa, x, tpl, tpr, cmp);
			}
		} while (--loop);
	}

	while (tpr > swap.begin() + 1 && tpl > array + 1)
	{
	tpr2:
		if (scandum_not_greater(cmp, *tpl, *(tpr - 1)))
		{
			*tpa-- = scandum_move(*tpr--); *tpa-- = scandum_move(*tpr--);

			if (tpr > swap.begin() + 1) { goto tpr2; } break;
		}

		if (scandum_greater(cmp, *(tpl - 1), *tpr))
		{
			*tpa-- = scandum_move(*tpl--); *tpa-- = scandum_move(*tpl--);

			if (tpl > array + 1) { goto tpl2; } break;
		}
		goto cross_swap;

	tpl2:
		if (scandum_greater(cmp, *(tpl - 1), *tpr))
		{
			*tpa-- = scandum_move(*tpl--); *tpa-- = scandum_move(*tpl--);

			if (tpl > array + 1) { goto tpl2; } break;
		}

		if (scandum_not_greater(cmp, *tpl, *(tpr - 1)))
		{
			*tpa-- = scandum_move(*tpr--); *tpa-- = scandum_move(*tpr--);

			if (tpr > swap.begin() + 1) { goto tpr2; } break;
		}
	cross_swap:

		x = scandum_not_greater(cmp, *tpl, *tpr);
		tpa--;
		tpa[x] = scandum_move(*tpr);
		tpr -= 1;
		tpa[!x] = scandum_move(*tpl);
		tpl -= 1;
		tpa--;
		scandum_tail_branchless_merge(tpa, x, tpl, tpr, cmp);
	}

	while (tpr >= swap.begin() && tpl >= array)
	{
		*tpa-- = scandum_move(scandum_greater(cmp, *tpl, *tpr) ? (T&)*tpl-- : (T&)*tpr--);
	}

	while (tpr >= swap.begin())
	{
		*tpa-- = scandum_move(*tpr--);
	}
}

// the next four functions provide in-place rotate merge support

template<typename T, typename Iterator>
void trinity_rotation(Iterator array, swap_space<T>& swap, size_t nmemb, size_t left)
{
	temp_var<T> temp;
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
					*--ptc = scandum_move(*--ptd); *ptd = scandum_move(*--ptb);
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
					temp = scandum_move(*--ptb);
					*ptb = scandum_move(*pta);
					*pta++ = scandum_move(*ptc);
					*ptc++ = scandum_move(*--ptd);
					*ptd = scandum_move(temp);
				}

				bridge = (ptd - ptc) / 2;

				while (bridge--)
				{
					temp = scandum_move(*ptc);
					*ptc++ = scandum_move(*--ptd);
					*ptd = scandum_move(*pta);
					*pta++ = scandum_move(temp);
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					temp = scandum_move(*pta);
					*pta++ = scandum_move(*--ptd);
					*ptd = scandum_move(temp);
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
					*ptc++ = scandum_move(*pta); *pta++ = scandum_move(*ptb++);
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
					temp = scandum_move(*--ptb);
					*ptb = scandum_move(*pta);
					*pta++ = scandum_move(*ptc);
					*ptc++ = scandum_move(*--ptd);
					*ptd = scandum_move(temp);
				}

				bridge = (ptb - pta) / 2;

				while (bridge--)
				{
					temp = scandum_move(*--ptb);
					*ptb = scandum_move(*pta);
					*pta++ = scandum_move(*--ptd);
					*ptd = scandum_move(temp);
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					temp = scandum_move(*pta);
					*pta++ = scandum_move(*--ptd);
					*ptd = scandum_move(temp);
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
			temp = scandum_move(*pta);
			*pta++ = scandum_move(*ptb);
			*ptb++ = scandum_move(temp);
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

		if (scandum_not_greater(cmp, *value, *(end - mid)))
		{
			end -= mid;
		}
		top -= mid;
	}

	if (scandum_not_greater(cmp, *value, *(end - 1)))
	{
		end--;
	}
	return (end - array);
}

template<typename T, typename Iterator, typename Compare>
void rotate_merge_block(Iterator array, swap_space<T>& swap, size_t lblock, size_t right, Compare cmp)
{
	size_t left, rblock, unbalanced;

	if (scandum_not_greater(cmp, *(array + lblock - 1), *(array + lblock)))
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
			trinity_rotation<T>(array + lblock, swap, rblock + left, rblock);

			unbalanced = (left * 2 < lblock) | (lblock * 2 < left);

			if (unbalanced && left <= swap.size())
			{
				partial_backward_merge<T>(array, swap, lblock + left, lblock, cmp);
			}
			else if (unbalanced && lblock <= swap.size())
			{
				partial_forward_merge<T>(array, swap, lblock + left, lblock, cmp);
			}
			else
			{
				rotate_merge_block<T>(array, swap, lblock, left, cmp);
			}
		}
	}

	if (right)
	{
		unbalanced = (right * 2 < rblock) | (rblock * 2 < right);

		if ((unbalanced && right <= swap.size()) || right + rblock <= swap.size())
		{
			partial_backward_merge<T>(array + lblock + left, swap, rblock + right, rblock, cmp);
		}
		else if (unbalanced && rblock <= swap.size())
		{
			partial_forward_merge<T>(array + lblock + left, swap, rblock + right, rblock, cmp);
		}
		else
		{
			rotate_merge_block<T>(array + lblock + left, swap, rblock, right, cmp);
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
		partial_backward_merge<T>(array, swap, nmemb, block, cmp);

		return;
	}

	while (block < nmemb)
	{
		for (pta = array ; pta + block < pte ; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				rotate_merge_block<T>(pta, swap, block, block, cmp);

				continue;
			}
			rotate_merge_block<T>(pta, swap, block, pte - pta - block, cmp);

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
		tail_swap<T>(pta, swap, nmemb, cmp);
	}
	else if (quad_swap<T>(pta, nmemb, cmp) == 0)
	{
		size_t block = quad_merge<T>(pta, swap, nmemb, 32, cmp);

		rotate_merge<T>(pta, swap, nmemb, block, cmp);
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

		detail::tail_swap<T>(pta, swap, nmemb, cmp);
	}
	else if (detail::quad_swap<T>(pta, nmemb, cmp) == 0)
	{
		size_t block, swap_size = nmemb;

		if (nmemb > 4194304) for (swap_size = 4194304 ; swap_size * 8 <= nmemb ; swap_size *= 4) {}

		detail::swap_space<T> swap(swap_size);

		block = detail::quad_merge<T>(pta, swap, nmemb, 32, cmp);

		detail::rotate_merge<T>(pta, swap, nmemb, block, cmp);
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
#undef scandum_branchless_swap
#undef scandum_swap_branchless
#undef scandum_move
#undef scandum_head_branchless_merge
#undef scandum_tail_branchless_merge
#undef scandum_parity_merge_two
#undef scandum_parity_merge_four

#endif
