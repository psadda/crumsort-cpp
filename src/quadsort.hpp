#ifndef SCANDUM_QUADSORT_HPP
#define SCANDUM_QUADSORT_HPP

// quadsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

#include <algorithm>   // for std::copy
#include <cassert>
#include <cstring>     // for std::memcpy, std::memmove, std::memset
#include <type_traits>

// universal copy functions to handle both trivially and nontrivially copyable types

#define scandum_copy_range(T, output, input, length) \
	if constexpr (std::is_trivially_copyable_v<T> && \
		std::is_same_v<decltype(input), T*> && \
		std::is_same_v<decltype(output), T*>) \
	{ \
		std::memcpy(output, input, (length) * sizeof(T)); \
	} \
	else \
	{ \
		std::copy(input, (input) + (length), output); \
	}

#define scandum_copy_overlapping_range(T, output, input, length) \
	if constexpr (std::is_trivially_copyable_v<T> && std::is_same_v<decltype(input), T*>) \
	{ \
		std::memmove(output, input, (length) * sizeof(T)); \
	} \
	else \
	{ \
		assert((output) < (input) || (output) > (input) + (length)); \
		std::copy_backward(input, (input) + (length), output); \
	}


// utilize branchless ternary operations in clang

#if !defined __clang__
#define scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp)  \
	x = cmp(*ptl, *ptr) <= 0;  \
	*ptd = *ptl;  \
	ptl += x;  \
	ptd[x] = *ptr;  \
	ptr += !x;  \
	ptd++;
#else
#define scandum_head_branchless_merge(ptd, x, ptl, ptr, cmp)  \
	*ptd++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;
#endif

#if !defined __clang__
#define scandum_tail_branchless_merge(tpd, y, tpl, tpr, cmp)  \
	y = cmp(*tpl, *tpr) <= 0;  \
	*tpd = *tpl;  \
	tpl -= !y;  \
	tpd--;  \
	tpd[y] = *tpr;  \
	tpr -= y;
#else
#define scandum_tail_branchless_merge(tpd, x, tpl, tpr, cmp)  \
	*tpd-- = cmp(*tpl, *tpr) > 0 ? *tpl-- : *tpr--;
#endif

// guarantee small parity merges are inlined with minimal overhead

#define scandum_parity_merge_two(array, swap, x, ptl, ptr, pts, cmp)  \
	ptl = array; ptr = array + 2; pts = swap;  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = cmp(*ptl, *ptr) <= 0 ? *ptl : *ptr;  \
  \
	ptl = array + 1; ptr = array + 3; pts = swap + 3;  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = cmp(*ptl, *ptr)  > 0 ? *ptl : *ptr;

#define scandum_parity_merge_four(array, swap, x, ptl, ptr, pts, cmp)  \
	ptl = array + 0; ptr = array + 4; pts = swap;  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_head_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = cmp(*ptl, *ptr) <= 0 ? *ptl : *ptr;  \
  \
	ptl = array + 3; ptr = array + 7; pts = swap + 7;  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	scandum_tail_branchless_merge(pts, x, ptl, ptr, cmp);  \
	*pts = cmp(*ptl, *ptr)  > 0 ? *ptl : *ptr;


#if !defined __clang__
#define scandum_branchless_swap(pta, swap, x, cmp)  \
	x = cmp(*pta, *(pta + 1)) > 0;  \
	swap = pta[!x];  \
	pta[0] = pta[x];  \
	pta[1] = swap;
#else
#define scandum_branchless_swap(pta, swap, x, cmp)  \
	x = 0;  \
	swap = cmp(*pta, *(pta + 1)) > 0 ? pta[x++] : pta[1];  \
	pta[0] = pta[x];  \
	pta[1] = swap;
#endif

#define scandum_swap_branchless(pta, swap, x, y, cmp)  \
	x = cmp(*pta, *(pta + 1)) > 0;  \
	y = !x;  \
	swap = pta[y];  \
	pta[0] = pta[x];  \
	pta[1] = swap;


namespace scandum {

namespace detail {

// the next seven functions are used for sorting 0 to 31 elements

template<typename Iterator, typename Compare>
void parity_swap_four(Iterator array, Compare cmp)
{
	typedef std::remove_reference_t<decltype(*array)> T;

	T tmp;
	Iterator pta = array;
	size_t x;

	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta--;

	if (cmp(*pta, *(pta + 1)) > 0)
	{
		tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp; pta--;

		scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
		scandum_branchless_swap(pta, tmp, x, cmp); pta--;
		scandum_branchless_swap(pta, tmp, x, cmp);
	}
}

template<typename Iterator, typename Compare>
void parity_swap_five(Iterator array, Compare cmp)
{
	typedef std::remove_reference_t<decltype(*array)> T;

	T tmp;
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
void parity_swap_six(Iterator array, T* swap, Compare cmp)
{
	T tmp;
	Iterator pta = array;
	T* ptl;
	T* ptr;
	size_t x, y;

	scandum_branchless_swap(pta, tmp, x, cmp); pta++;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 3;
	scandum_branchless_swap(pta, tmp, x, cmp); pta--;
	scandum_branchless_swap(pta, tmp, x, cmp); pta = array;

	if (cmp(*(pta + 2), *(pta + 3)) <= 0)
	{
		scandum_branchless_swap(pta, tmp, x, cmp); pta += 4;
		scandum_branchless_swap(pta, tmp, x, cmp);
		return;
	}
	x = cmp(*pta, *(pta + 1)) > 0; y = !x; swap[0] = pta[x]; swap[1] = pta[y]; swap[2] = pta[2]; pta += 4;
	x = cmp(*pta, *(pta + 1)) > 0; y = !x; swap[4] = pta[x]; swap[5] = pta[y]; swap[3] = pta[-1];

	pta = array; ptl = swap; ptr = swap + 3;

	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);

	pta = array + 5; ptl = swap + 2; ptr = swap + 5;

	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	*pta = cmp(*ptl, *ptr)  > 0 ? *ptl : *ptr;
}

template<typename T, typename Iterator, typename Compare>
void parity_swap_seven(Iterator array, T* swap, Compare cmp)
{
	T tmp;
	Iterator pta = array;
	T* ptl;
	T* ptr;
	size_t x, y;

	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta -= 3;
	scandum_branchless_swap(pta, tmp, y, cmp); pta += 2;
	scandum_branchless_swap(pta, tmp, x, cmp); pta += 2; y += x;
	scandum_branchless_swap(pta, tmp, x, cmp); pta -= 1; y += x;

	if (y == 0) return;

	scandum_branchless_swap(pta, tmp, x, cmp); pta = array;

	x = cmp(*pta, *(pta + 1)) > 0; swap[0] = pta[x]; swap[1] = pta[!x]; swap[2] = pta[2]; pta += 3;
	x = cmp(*pta, *(pta + 1)) > 0; swap[3] = pta[x]; swap[4] = pta[!x]; pta += 2;
	x = cmp(*pta, *(pta + 1)) > 0; swap[5] = pta[x]; swap[6] = pta[!x];

	pta = array; ptl = swap; ptr = swap + 3;

	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);
	scandum_head_branchless_merge(pta, x, ptl, ptr, cmp);

	pta = array + 6; ptl = swap + 2; ptr = swap + 6;

	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	scandum_tail_branchless_merge(pta, y, ptl, ptr, cmp);
	*pta = cmp(*ptl, *ptr) > 0 ? *ptl : *ptr;
}

template<typename T, typename Iterator, typename Compare>
void tiny_sort(Iterator array, T* swap, size_t nmemb, Compare cmp)
{
	T tmp;
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
			parity_swap_four(array, cmp);
			return;
		case 5:
			parity_swap_five(array, cmp);
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

template<typename OutputIt, typename InputIt, typename Compare>
void parity_merge(OutputIt dest, InputIt from, size_t left, size_t right, Compare cmp)
{
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*dest)>> OutputT;
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*from)>> InputT;
	static_assert (
		std::is_same_v<OutputT, InputT>,
		"output and input iterators must refer to the same type"
	);

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
		*ptd++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;
	}
	*ptd++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;

#if !defined cmp && !defined __clang__ // cache limit workaround for gcc
	if (left > QUAD_CACHE)
	{
		while (--left)
		{
			*ptd++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;
			*tpd-- = cmp(*tpl, *tpr)  > 0 ? *tpl-- : *tpr--;
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
	*tpd = cmp(*tpl, *tpr)  > 0 ? *tpl : *tpr;
}

template<typename T, typename Iterator, typename Compare>
void tail_swap(Iterator array, T* swap, size_t nmemb, Compare cmp)
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

	if (cmp(*(array + quad1 - 1), *(array + quad1)) <= 0 && cmp(*(array + half1 - 1), *(array + half1)) <= 0 && cmp(*(pta - 1), *pta) <= 0)
	{
		return;
	}
	parity_merge(swap, array, quad1, quad2, cmp);
	parity_merge(swap + half1, array + half1, quad3, quad4, cmp);
	parity_merge(array, swap, half1, half2, cmp);
}

// the next three functions create sorted blocks of 32 elements

template<typename Iterator>
void quad_reversal(Iterator pta, Iterator ptz)
{
	typedef std::remove_reference_t<decltype(*pta)> T;

	Iterator ptb, pty;
	T tmp1, tmp2;

	size_t loop = (ptz - pta) / 2;

	ptb = pta + loop;
	pty = ptz - loop;

	if (loop % 2 == 0)
	{
		tmp2 = *ptb; *ptb-- = *pty; *pty++ = tmp2; loop--;
	}

	loop /= 2;

	do
	{
		tmp1 = *pta; *pta++ = *ptz; *ptz-- = tmp1;
		tmp2 = *ptb; *ptb-- = *pty; *pty++ = tmp2;
	}
	while (loop--);
}

template<typename T, typename Iterator, typename Compare>
void quad_swap_merge(Iterator array, T* swap, Compare cmp)
{
	T* pts;
	Iterator ptl;
	Iterator ptr;
#if !defined __clang__
	size_t x;
#endif
	scandum_parity_merge_two(array + 0, swap + 0, x, ptl, ptr, pts, cmp);
	scandum_parity_merge_two(array + 4, swap + 4, x, ptl, ptr, pts, cmp);

	Iterator pts2;
	T* ptl2;
	T* ptr2;
	scandum_parity_merge_four(swap, array, x, ptl2, ptr2, pts2, cmp);
}

template<typename T, typename Iterator, typename Compare>
void tail_merge(Iterator array, T* swap, size_t swap_size, size_t nmemb, size_t block, Compare cmp)
{
	Iterator pta, pte;

	pte = array + nmemb;

	while (block < nmemb && block <= swap_size)
	{
		for (pta = array; pta + block < pte; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				partial_backward_merge(pta, swap, swap_size, block * 2, block, cmp);

				continue;
			}
			partial_backward_merge(pta, swap, swap_size, pte - pta, block, cmp);

			break;
		}
		block *= 2;
	}
}
template<typename Iterator, typename Compare>
size_t quad_swap(Iterator array, size_t nmemb, Compare cmp)
{
	typedef std::remove_reference_t<decltype(*array)> T;

	T tmp, swap[32];
	size_t count;
	Iterator pta, pts;
	unsigned char v1, v2, v3, v4, x;
	pta = array;

	count = nmemb / 8;

	while (count--)
	{
		v1 = cmp(*(pta + 0), *(pta + 1)) > 0;
		v2 = cmp(*(pta + 2), *(pta + 3)) > 0;
		v3 = cmp(*(pta + 4), *(pta + 5)) > 0;
		v4 = cmp(*(pta + 6), *(pta + 7)) > 0;

		switch (v1 + v2 * 2 + v3 * 4 + v4 * 8)
		{
			case 0:
				if (cmp(*(pta + 1), *(pta + 2)) <= 0 && cmp(*(pta + 3), *(pta + 4)) <= 0 && cmp(*(pta + 5), *(pta + 6)) <= 0)
				{
					goto ordered;
				}
				quad_swap_merge(pta, swap, cmp);
				break;

			case 15:
				if (cmp(*(pta + 1), *(pta + 2)) > 0 && cmp(*(pta + 3), *(pta + 4)) > 0 && cmp(*(pta + 5), *(pta + 6)) > 0)
				{
					pts = pta;
					goto reversed;
				}

			default:
			not_ordered:
				x = !v1; tmp = pta[x]; pta[0] = pta[v1]; pta[1] = tmp; pta += 2;
				x = !v2; tmp = pta[x]; pta[0] = pta[v2]; pta[1] = tmp; pta += 2;
				x = !v3; tmp = pta[x]; pta[0] = pta[v3]; pta[1] = tmp; pta += 2;
				x = !v4; tmp = pta[x]; pta[0] = pta[v4]; pta[1] = tmp; pta -= 6;

				quad_swap_merge(pta, swap, cmp);
		}
		pta += 8;

		continue;

		ordered:

		pta += 8;

		if (count--)
		{
			if ((v1 = cmp(*(pta + 0), *(pta + 1)) > 0) | (v2 = cmp(*(pta + 2), *(pta + 3)) > 0) | (v3 = cmp(*(pta + 4), *(pta + 5)) > 0) | (v4 = cmp(*(pta + 6), *(pta + 7)) > 0))
			{
				if (v1 + v2 + v3 + v4 == 4 && cmp(*(pta + 1), *(pta + 2)) > 0 && cmp(*(pta + 3), *(pta + 4)) > 0 && cmp(*(pta + 5), *(pta + 6)) > 0)
				{
					pts = pta;
					goto reversed;
				}
				goto not_ordered;
			}
			if (cmp(*(pta + 1), *(pta + 2)) <= 0 && cmp(*(pta + 3), *(pta + 4)) <= 0 && cmp(*(pta + 5), *(pta + 6)) <= 0)
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
			if ((v1 = cmp(*(pta + 0), *(pta + 1)) <= 0) | (v2 = cmp(*(pta + 2), *(pta + 3)) <= 0) | (v3 = cmp(*(pta + 4), *(pta + 5)) <= 0) | (v4 = cmp(*(pta + 6), *(pta + 7)) <= 0))
			{
				// not reversed
			}
			else
			{
				if (cmp(*(pta - 1), *pta) > 0 && cmp(*(pta + 1), *(pta + 2)) > 0 && cmp(*(pta + 3), *(pta + 4)) > 0 && cmp(*(pta + 5), *(pta + 6)) > 0)
				{
					goto reversed;
				}
			}
			quad_reversal(pts, pta - 1);

			if (v1 + v2 + v3 + v4 == 4 && cmp(*(pta + 1), *(pta + 2)) <= 0 && cmp(*(pta + 3), *(pta + 4)) <= 0 && cmp(*(pta + 5), *(pta + 6)) <= 0)
			{
				goto ordered;
			}
			if (v1 + v2 + v3 + v4 == 0 && cmp(*(pta + 1), *(pta + 2))  > 0 && cmp(*(pta + 3), *(pta + 4))  > 0 && cmp(*(pta + 5), *(pta + 6))  > 0)
			{
				pts = pta;
				goto reversed;
			}

			x = !v1; tmp = pta[v1]; pta[0] = pta[x]; pta[1] = tmp; pta += 2;
			x = !v2; tmp = pta[v2]; pta[0] = pta[x]; pta[1] = tmp; pta += 2;
			x = !v3; tmp = pta[v3]; pta[0] = pta[x]; pta[1] = tmp; pta += 2;
			x = !v4; tmp = pta[v4]; pta[0] = pta[x]; pta[1] = tmp; pta -= 6;

			if (cmp(*(pta + 1), *(pta + 2)) > 0 || cmp(*(pta + 3), *(pta + 4)) > 0 || cmp(*(pta + 5), *(pta + 6)) > 0)
			{
				quad_swap_merge(pta, swap, cmp);
			}
			pta += 8;
			continue;
		}

		switch (nmemb % 8)
		{
			case 7: if (cmp(*(pta + 5), *(pta + 6)) <= 0) break;
			case 6: if (cmp(*(pta + 4), *(pta + 5)) <= 0) break;
			case 5: if (cmp(*(pta + 3), *(pta + 4)) <= 0) break;
			case 4: if (cmp(*(pta + 2), *(pta + 3)) <= 0) break;
			case 3: if (cmp(*(pta + 1), *(pta + 2)) <= 0) break;
			case 2: if (cmp(*(pta + 0), *(pta + 1)) <= 0) break;
			case 1: if (cmp(*(pta - 1), *(pta + 0)) <= 0) break;
			case 0:
				quad_reversal(pts, pta + nmemb % 8 - 1);

				if (pts == array)
				{
					return 1;
				}
				goto reverse_end;
		}
		quad_reversal(pts, pta - 1);
		break;
	}
	tail_swap(pta, swap, nmemb % 8, cmp);

	reverse_end:

	pta = array;

	for (count = nmemb / 32 ; count-- ; pta += 32)
	{
		if (cmp(*(pta + 7), *(pta + 8)) <= 0 && cmp(*(pta + 15), *(pta + 16)) <= 0 && cmp(*(pta + 23), *(pta + 24)) <= 0)
		{
			continue;
		}
		parity_merge(swap, pta, 8, 8, cmp);
		parity_merge(swap + 16, pta + 16, 8, 8, cmp);
		parity_merge(pta, swap, 16, 16, cmp);
	}

	if (nmemb % 32 > 8)
	{
		tail_merge(pta, swap, 32, nmemb % 32, 8, cmp);
	}
	return 0;
}

// The next six functions are quad merge support routines

template<typename OutputIt, typename InputIt, typename Compare>
void cross_merge(OutputIt dest, InputIt from, size_t left, size_t right, Compare cmp)
{
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*dest)>> OutputT;
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*from)>> InputT;
	static_assert (
		std::is_same_v<OutputT, InputT>,
		"output and input iterators must refer to the same type"
	);
	typedef std::remove_cv_t<std::remove_reference_t<decltype(*dest)>> T;

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
		if (cmp(*(ptl + 15), *ptr) > 0 && cmp(*ptl, *(ptr + 15)) <= 0 && cmp(*tpl, *(tpr - 15)) > 0 && cmp(*(tpl - 15), *tpr) <= 0)
		{
			parity_merge(dest, from, left, right, cmp);
			return;
		}
	}
	OutputIt ptd = dest;
	OutputIt tpd = dest + left + right - 1;

	while (1)
	{
		if (tpl - ptl > 8)
		{
			ptl8_ptr: if (cmp(*(ptl + 7), *ptr) <= 0)
			{
				scandum_copy_range(T, ptd, ptl, 8); ptd += 8; ptl += 8;

				if (tpl - ptl > 8) {goto ptl8_ptr;} continue;
			}

			tpl8_tpr: if (cmp(*(tpl - 7), *tpr) > 0)
			{
				tpd -= 7; tpl -= 7; scandum_copy_range(T, tpd--, tpl--, 8);

				if (tpl - ptl > 8) {goto tpl8_tpr;} continue;
			}
		}

		if (tpr - ptr > 8)
		{
			ptl_ptr8: if (cmp(*ptl, *(ptr + 7)) > 0)
			{
				scandum_copy_range(T, ptd, ptr, 8); ptd += 8; ptr += 8;

				if (tpr - ptr > 8) {goto ptl_ptr8;} continue;
			}

			tpl_tpr8: if (cmp(*tpl, *(tpr - 7)) <= 0)
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
				*ptd++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;
				*tpd-- = cmp(*tpl, *tpr)  > 0 ? *tpl-- : *tpr--;
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
		*ptd++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;
	}
	while (ptl <= tpl)
	{
		*ptd++ = *ptl++;
	}
	while (ptr <= tpr)
	{
		*ptd++ = *ptr++;
	}
}

template<typename T, typename Iterator, typename Compare>
void quad_merge_block(Iterator array, T* swap, size_t block, Compare cmp)
{
	Iterator pt1, pt2, pt3;
	size_t block_x_2 = block * 2;

	pt1 = array + block;
	pt2 = pt1 + block;
	pt3 = pt2 + block;

	switch ((cmp(*(pt1 - 1), *pt1) <= 0) | (cmp(*(pt3 - 1), *pt3) <= 0) * 2)
	{
		case 0:
			cross_merge(swap, array, block, block, cmp);
			cross_merge(swap + block_x_2, pt2, block, block, cmp);
			break;
		case 1:
			scandum_copy_range(T, swap, array, block_x_2);
			cross_merge(swap + block_x_2, pt2, block, block, cmp);
			break;
		case 2:
			cross_merge(swap, array, block, block, cmp);
			scandum_copy_range(T, swap + block_x_2, pt2, block_x_2);
			break;
		case 3:
			if (cmp(*(pt2 - 1), *pt2) <= 0)
				return;
			scandum_copy_range(T, swap, array, block_x_2 * 2);
	}
	cross_merge(array, swap, block_x_2, block_x_2, cmp);
}

template<typename T, typename Iterator, typename Compare>
size_t quad_merge(Iterator array, T* swap, size_t swap_size, size_t nmemb, size_t block, Compare cmp)
{
	Iterator pta, pte;

	pte = array + nmemb;

	block *= 4;

	while (block <= nmemb && block <= swap_size)
	{
		pta = array;

		do
		{
			quad_merge_block(pta, swap, block / 4, cmp);

			pta += block;
		}
		while (pta + block <= pte);

		tail_merge(pta, swap, swap_size, pte - pta, block / 4, cmp);

		block *= 4;
	}

	tail_merge(array, swap, swap_size, nmemb, block / 4, cmp);

	return block / 2;
}

template<typename T, typename Iterator, typename Compare>
void partial_forward_merge(Iterator array, T* swap, size_t swap_size, size_t nmemb, size_t block, Compare cmp)
{
	T *ptl, *tpl;
	Iterator ptr, tpr;
	size_t x;

	if (nmemb == block)
	{
		return;
	}

	ptr = array + block;
	tpr = array + nmemb - 1;

	if (cmp(*(ptr - 1), *ptr) <= 0)
	{
		return;
	}

	scandum_copy_range(T, swap, array, block);

	ptl = swap;
	tpl = swap + block - 1;

	while (ptl < tpl - 1 && ptr < tpr - 1)
	{
		ptr2: if (cmp(*ptl, *(ptr + 1)) > 0)
		{
			*array++ = *ptr++; *array++ = *ptr++;

			if (ptr < tpr - 1) {goto ptr2;} break;
		}
		if (cmp(*(ptl + 1), *ptr) <= 0)
		{
			*array++ = *ptl++; *array++ = *ptl++;

			if (ptl < tpl - 1) {goto ptl2;} break;
		}

		goto cross_swap;

		ptl2: if (cmp(*(ptl + 1), *ptr) <= 0)
		{
			*array++ = *ptl++; *array++ = *ptl++;

			if (ptl < tpl - 1) {goto ptl2;} break;
		}

		if (cmp(*ptl, *(ptr + 1)) > 0)
		{
			*array++ = *ptr++; *array++ = *ptr++;

			if (ptr < tpr - 1) {goto ptr2;} break;
		}

		cross_swap:

		x = cmp(*ptl, *ptr) <= 0; array[x] = *ptr; ptr += 1; array[!x] = *ptl; ptl += 1; array += 2;
		scandum_head_branchless_merge(array, x, ptl, ptr, cmp);
	}

	while (ptl <= tpl && ptr <= tpr)
	{
		*array++ = cmp(*ptl, *ptr) <= 0 ? *ptl++ : *ptr++;
	}

	while (ptl <= tpl)
	{
		*array++ = *ptl++;
	}
}

template<typename T, typename Iterator, typename Compare>
void partial_backward_merge(Iterator array, T* swap, size_t swap_size, size_t nmemb, size_t block, Compare cmp)
{
	Iterator tpl, tpa;
	T* tpr;
	size_t right, loop, x;

	if (nmemb == block)
	{
		return;
	}

	tpl = array + block - 1;
	tpa = array + nmemb - 1;

	if (cmp(*tpl, *(tpl + 1)) <= 0)
	{
		return;
	}

	right = nmemb - block;

	if (nmemb <= swap_size && right >= 64)
	{
		cross_merge(swap, array, block, right, cmp);

		scandum_copy_range(T, array, swap, nmemb);

		return;
	}

	scandum_copy_range(T, swap, array + block, right);

	tpr = swap + right - 1;

	while (tpl > array + 16 && tpr > swap + 16)
	{
		tpl_tpr16: if (cmp(*tpl, *(tpr - 15)) <= 0)
		{
			loop = 16; do *tpa-- = *tpr--; while (--loop);

			if (tpr > swap + 16) {goto tpl_tpr16;} break;
		}

		tpl16_tpr: if (cmp(*(tpl - 15), *tpr) > 0)
		{
			loop = 16; do *tpa-- = *tpl--; while (--loop);
			
			if (tpl > array + 16) {goto tpl16_tpr;} break;
		}
		loop = 8; do
		{
			if (cmp(*tpl, *(tpr - 1)) <= 0)
			{
				*tpa-- = *tpr--; *tpa-- = *tpr--;
			}
			else if (cmp(*(tpl - 1), *tpr) > 0)
			{
				*tpa-- = *tpl--; *tpa-- = *tpl--;
			}
			else
			{
				x = cmp(*tpl, *tpr) <= 0; tpa--; tpa[x] = *tpr; tpr -= 1; tpa[!x] = *tpl; tpl -= 1; tpa--;
				scandum_tail_branchless_merge(tpa, x, tpl, tpr, cmp);
			}
		}
		while (--loop);
	}

	while (tpr > swap + 1 && tpl > array + 1)
	{
		tpr2: if (cmp(*tpl, *(tpr - 1)) <= 0)
		{
			*tpa-- = *tpr--; *tpa-- = *tpr--;
			
			if (tpr > swap + 1) {goto tpr2;} break;
		}

		if (cmp(*(tpl - 1), *tpr) > 0)
		{
			*tpa-- = *tpl--; *tpa-- = *tpl--;

			if (tpl > array + 1) { goto tpl2; } break;
		}
		goto cross_swap;

	tpl2: if (cmp(*(tpl - 1), *tpr) > 0)
		{
			*tpa-- = *tpl--; *tpa-- = *tpl--;

			if (tpl > array + 1) {goto tpl2;} break;
		}

		if (cmp(*tpl, *(tpr - 1)) <= 0)
		{
			*tpa-- = *tpr--; *tpa-- = *tpr--;
			
			if (tpr > swap + 1) {goto tpr2;} break;
		}
		cross_swap:

		x = cmp(*tpl, *tpr) <= 0; tpa--; tpa[x] = *tpr; tpr -= 1; tpa[!x] = *tpl; tpl -= 1; tpa--;
		scandum_tail_branchless_merge(tpa, x, tpl, tpr, cmp);
	}

	while (tpr >= swap && tpl >= array)
	{
		*tpa-- = cmp(*tpl, *tpr) > 0 ? *tpl-- : *tpr--;
	}

	while (tpr >= swap)
	{
		*tpa-- = *tpr--;
	}
}

// the next four functions provide in-place rotate merge support

template<typename T, typename Iterator>
void trinity_rotation(Iterator array, T* swap, size_t swap_size, size_t nmemb, size_t left)
{
	T temp;
	size_t bridge, right = nmemb - left;

	if (swap_size > 65536)
	{
		swap_size = 65536;
	}

	if (left < right)
	{
		if (left <= swap_size)
		{
			scandum_copy_range(T, swap, array, left);
			scandum_copy_overlapping_range(T, array, array + left, right);
			scandum_copy_range(T, array + right, swap, left);
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

				scandum_copy_range(T, swap, ptb, bridge);

				while (left--)
				{
					*--ptc = *--ptd; *ptd = *--ptb;
				}
				scandum_copy_range(T, pta, swap, bridge);
			}
			else
			{
				ptc = ptb;
				ptd = ptc + right;

				bridge = left / 2;

				while (bridge--)
				{
					temp = *--ptb; *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = temp;
				}

				bridge = (ptd - ptc) / 2;

				while (bridge--)
				{
					temp = *ptc; *ptc++ = *--ptd; *ptd = *pta; *pta++ = temp;
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					temp = *pta; *pta++ = *--ptd; *ptd = temp;
				}
			}
		}
	}
	else if (right < left)
	{
		if (right <= swap_size)
		{
			scandum_copy_range(T, swap, array + left, right);
			scandum_copy_overlapping_range(T, array + right, array, left);
			scandum_copy_range(T, array, swap, right);
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

				scandum_copy_range(T, swap, ptc, bridge);

				while (right--)
				{
					*ptc++ = *pta; *pta++ = *ptb++;
				}
				scandum_copy_range(T, ptd - bridge, swap, bridge);
			}
			else
			{
				ptc = ptb;
				ptd = ptc + right;

				bridge = right / 2;

				while (bridge--)
				{
					temp = *--ptb; *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = temp;
				}

				bridge = (ptb - pta) / 2;

				while (bridge--)
				{
					temp = *--ptb; *ptb = *pta; *pta++ = *--ptd; *ptd = temp;
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					temp = *pta; *pta++ = *--ptd; *ptd = temp;
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
			temp = *pta; *pta++ = *ptb; *ptb++ = temp;
		}
	}
}

template<typename Iterator, typename Compare>
size_t monobound_binary_first(Iterator array, Iterator value, size_t top, Compare cmp)
{
	size_t mid;
	Iterator end = array + top;

	while (top > 1)
	{
		mid = top / 2;

		if (cmp(*value, *(end - mid)) <= 0)
		{
			end -= mid;
		}
		top -= mid;
	}

	if (cmp(*value, *(end - 1)) <= 0)
	{
		end--;
	}
	return (end - array);
}

template<typename T, typename Iterator, typename Compare>
void rotate_merge_block(Iterator array, T* swap, size_t swap_size, size_t lblock, size_t right, Compare cmp)
{
	size_t left, rblock, unbalanced;

	if (cmp(*(array + lblock - 1), *(array + lblock)) <= 0)
	{
		return;
	}

	rblock = lblock / 2;
	lblock -= rblock;

	left = monobound_binary_first(array + lblock + rblock, array + lblock, right, cmp);

	right -= left;

	// [ lblock ] [ rblock ] [ left ] [ right ]

	if (left)
	{
		if (lblock + left <= swap_size)
		{
			scandum_copy_range(T, swap, array, lblock);
			scandum_copy_range(T, swap + lblock, array + lblock + rblock, left);
			scandum_copy_overlapping_range(T, array + lblock + left, array + lblock, rblock);

			cross_merge(array, swap, lblock, left, cmp);
		}
		else
		{
			trinity_rotation(array + lblock, swap, swap_size, rblock + left, rblock);

			unbalanced = (left * 2 < lblock) | (lblock * 2 < left);

			if (unbalanced && left <= swap_size)
			{
				partial_backward_merge(array, swap, swap_size, lblock + left, lblock, cmp);
			}
			else if (unbalanced && lblock <= swap_size)
			{
				partial_forward_merge(array, swap, swap_size, lblock + left, lblock, cmp);
			}
			else
			{
				rotate_merge_block(array, swap, swap_size, lblock, left, cmp);
			}
		}
	}

	if (right)
	{
		unbalanced = (right * 2 < rblock) | (rblock * 2 < right);

		if ((unbalanced && right <= swap_size) || right + rblock <= swap_size)
		{
			partial_backward_merge(array + lblock + left, swap, swap_size, rblock + right, rblock, cmp);
		}
		else if (unbalanced && rblock <= swap_size)
		{
			partial_forward_merge(array + lblock + left, swap, swap_size, rblock + right, rblock, cmp);
		}
		else
		{
			rotate_merge_block(array + lblock + left, swap, swap_size, rblock, right, cmp);
		}
	}
}

template<typename T, typename Iterator, typename Compare>
void rotate_merge(Iterator array, T* swap, size_t swap_size, size_t nmemb, size_t block, Compare cmp)
{
	Iterator pta, pte;

	pte = array + nmemb;

	if (nmemb <= block * 2 && nmemb - block <= swap_size)
	{
		partial_backward_merge(array, swap, swap_size, nmemb, block, cmp);

		return;
	}

	while (block < nmemb)
	{
		for (pta = array ; pta + block < pte ; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				rotate_merge_block(pta, swap, swap_size, block, block, cmp);

				continue;
			}
			rotate_merge_block(pta, swap, swap_size, block, pte - pta - block, cmp);

			break;
		}
		block *= 2;
	}
}

template<typename T, typename Iterator, typename Compare>
void quadsort_swap(Iterator array, T* swap, size_t swap_size, size_t nmemb, Compare cmp)
{
	Iterator pta = array;
	T* pts = swap;

	if (nmemb <= 96)
	{
		tail_swap(pta, pts, nmemb, cmp);
	}
	else if (quad_swap(pta, nmemb, cmp) == 0)
	{
		size_t block = quad_merge(pta, pts, swap_size, nmemb, 32, cmp);

		rotate_merge(pta, pts, swap_size, nmemb, block, cmp);
	}
}

} // namespace scandum::detail

template<typename T, typename Compare>
void quadsort(T* array, size_t nmemb, Compare cmp)
{
	T* pta = (T*)array;

	if (nmemb < 32)
	{
		T swap[nmemb];

		detail::tail_swap(pta, swap, nmemb, cmp);
	}
	else if (detail::quad_swap(pta, nmemb, cmp) == 0)
	{
		T* swap = NULL;
		size_t block, swap_size = nmemb;

		if (nmemb > 4194304) for (swap_size = 4194304 ; swap_size * 8 <= nmemb ; swap_size *= 4) {}

		swap = (T*)std::malloc(swap_size * sizeof(T));

		if (swap == NULL)
		{
			T stack[512];

			block = detail::quad_merge(pta, stack, 512, nmemb, 32, cmp);

			detail::rotate_merge(pta, stack, 512, nmemb, block, cmp);

			return;
		}
		block = detail::quad_merge(pta, swap, swap_size, nmemb, 32, cmp);

		detail::rotate_merge(pta, swap, swap_size, nmemb, block, cmp);

		std::free(swap);
	}
}

} // namespace scandum


#undef scandum_head_branchless_merge
#undef scandum_tail_branchless_merge
#undef scandum_parity_merge_two
#undef scandum_parity_merge_four
#undef scandum_branchless_swap
#undef scandum_swap_branchless

#endif
