#ifndef SCANDUM_CRUMSORT_HPP
#define SCANDUM_CRUMSORT_HPP

// crumsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

// When sorting an array of pointers, like a string array, the QUAD_CACHE needs
// to be set for proper performance when sorting large arrays.
// crumsort_prim() can be used to sort arrays of 32 and 64 bit integers
// without a comparison function or cache restrictions.

// With a 6 MB L3 cache a value of 262144 works well.

#ifdef cmp
#define QUAD_CACHE 4294967295
#else
//#define QUAD_CACHE 131072
#define QUAD_CACHE 262144
//#define QUAD_CACHE 524288
//#define QUAD_CACHE 4294967295
#endif

#include "quadsort.hpp"

#define CRUM_AUX  512
#define CRUM_OUT   96

namespace scandum {

namespace detail {

template<typename T, typename Iterator, typename Compare>
void fulcrum_partition(Iterator array, T* swap, T* max, size_t swap_size, size_t nmemb, Compare cmp);

template<typename T, typename Iterator, typename Compare>
void crum_analyze(Iterator array, T* swap, size_t swap_size, size_t nmemb, Compare cmp)
{
	unsigned char loop, asum, bsum, csum, dsum;
	unsigned int astreaks, bstreaks, cstreaks, dstreaks;
	size_t quad1, quad2, quad3, quad4, half1, half2;
	size_t cnt, abalance, bbalance, cbalance, dbalance;
	Iterator pta, ptb, ptc, ptd;

	half1 = nmemb / 2;
	quad1 = half1 / 2;
	quad2 = half1 - quad1;
	half2 = nmemb - half1;
	quad3 = half2 / 2;
	quad4 = half2 - quad3;

	pta = array;
	ptb = array + quad1;
	ptc = array + half1;
	ptd = array + half1 + quad3;

	astreaks = bstreaks = cstreaks = dstreaks = 0;
	abalance = bbalance = cbalance = dbalance = 0;

	for (cnt = nmemb ; cnt > 132 ; cnt -= 128)
	{
		for (asum = bsum = csum = dsum = 0, loop = 32 ; loop ; loop--)
		{
			asum += cmp(*pta, *(pta + 1)) == false; pta++;
			bsum += cmp(*ptb, *(ptb + 1)) == false; ptb++;
			csum += cmp(*ptc, *(ptc + 1)) == false; ptc++;
			dsum += cmp(*ptd, *(ptd + 1)) == false; ptd++;
		}
		abalance += asum; astreaks += asum = (asum == 0) | (asum == 32);
		bbalance += bsum; bstreaks += bsum = (bsum == 0) | (bsum == 32);
		cbalance += csum; cstreaks += csum = (csum == 0) | (csum == 32);
		dbalance += dsum; dstreaks += dsum = (dsum == 0) | (dsum == 32);

		if (cnt > 516 && asum + bsum + csum + dsum == 0)
		{
			abalance += 48; pta += 96;
			bbalance += 48; ptb += 96;
			cbalance += 48; ptc += 96;
			dbalance += 48; ptd += 96;
			cnt -= 384;
		}
	}

	for ( ; cnt > 7 ; cnt -= 4)
	{
		abalance += cmp(*pta, *(pta + 1)) == false; pta++;
		bbalance += cmp(*ptb, *(ptb + 1)) == false; ptb++;
		cbalance += cmp(*ptc, *(ptc + 1)) == false; ptc++;
		dbalance += cmp(*ptd, *(ptd + 1)) == false; ptd++;
	}

	if (quad1 < quad2) {bbalance += cmp(*ptb, *(ptb + 1)) == false; ptb++;}
	if (quad1 < quad3) {cbalance += cmp(*ptc, *(ptc + 1)) == false; ptc++;}
	if (quad1 < quad4) {dbalance += cmp(*ptd, *(ptd + 1)) == false; ptd++;}

	cnt = abalance + bbalance + cbalance + dbalance;

	if (cnt == 0)
	{
		if (cmp(*pta, *(pta + 1)) == true && cmp(*ptb, *(ptb + 1)) == true && cmp(*ptc, *(ptc + 1)) == true)
		{
			return;
		}
	}

	asum = quad1 - abalance == 1;
	bsum = quad2 - bbalance == 1;
	csum = quad3 - cbalance == 1;
	dsum = quad4 - dbalance == 1;

	if (asum | bsum | csum | dsum)
	{
		unsigned char span1 = (asum && bsum) * (cmp(*pta, *(pta + 1)) == false);
		unsigned char span2 = (bsum && csum) * (cmp(*ptb, *(ptb + 1)) == false);
		unsigned char span3 = (csum && dsum) * (cmp(*ptc, *(ptc + 1)) == false);

		switch (span1 | span2 * 2 | span3 * 4)
		{
			case 0: break;
			case 1: quad_reversal(array, ptb);   abalance = bbalance = 0; break;
			case 2: quad_reversal(pta + 1, ptc); bbalance = cbalance = 0; break;
			case 3: quad_reversal(array, ptc);   abalance = bbalance = cbalance = 0; break;
			case 4: quad_reversal(ptb + 1, ptd); cbalance = dbalance = 0; break;
			case 5: quad_reversal(array, ptb);
				quad_reversal(ptb + 1, ptd); abalance = bbalance = cbalance = dbalance = 0; break;
			case 6: quad_reversal(pta + 1, ptd); bbalance = cbalance = dbalance = 0; break;
			case 7: quad_reversal(array, ptd); return;
		}

		if (asum && abalance) {quad_reversal(array,   pta); abalance = 0;}
		if (bsum && bbalance) {quad_reversal(pta + 1, ptb); bbalance = 0;}
		if (csum && cbalance) {quad_reversal(ptb + 1, ptc); cbalance = 0;}
		if (dsum && dbalance) {quad_reversal(ptc + 1, ptd); dbalance = 0;}
	}

#ifdef cmp
	cnt = nmemb / 256; // switch to quadsort if at least 50% ordered
#else
	cnt = nmemb / 512; // switch to quadsort if at least 25% ordered
#endif
	asum = astreaks > cnt;
	bsum = bstreaks > cnt;
	csum = cstreaks > cnt;
	dsum = dstreaks > cnt;

#ifndef cmp
	if (quad1 > QUAD_CACHE)
	{
//		asum = bsum = csum = dsum = 1;
		goto quad_cache;
	}
#endif

	switch (asum + bsum * 2 + csum * 4 + dsum * 8)
	{
		case 0:
			fulcrum_partition(array, swap, (T*)nullptr, swap_size, nmemb, cmp);
			return;
		case 1:
			if (abalance) quadsort_swap(array, swap, swap_size, quad1, cmp);
			fulcrum_partition(pta + 1, swap, (T*)nullptr, swap_size, quad2 + half2, cmp);
			break;
		case 2:
			fulcrum_partition(array, swap, (T*)nullptr, swap_size, quad1, cmp);
			if (bbalance) quadsort_swap(pta + 1, swap, swap_size, quad2, cmp);
			fulcrum_partition(ptb + 1, swap, (T*)nullptr, swap_size, half2, cmp);
			break;
		case 3:
			if (abalance) quadsort_swap(array, swap, swap_size, quad1, cmp);
			if (bbalance) quadsort_swap(pta + 1, swap, swap_size, quad2, cmp);
			fulcrum_partition(ptb + 1, swap, (T*)nullptr, swap_size, half2, cmp);
			break;
		case 4:
			fulcrum_partition(array, swap, (T*)nullptr, swap_size, half1, cmp);
			if (cbalance) quadsort_swap(ptb + 1, swap, swap_size, quad3, cmp);
			fulcrum_partition(ptc + 1, swap, (T*)nullptr, swap_size, quad4, cmp);
			break;
		case 8:
			fulcrum_partition(array, swap, (T*)nullptr, swap_size, half1 + quad3, cmp);
			if (dbalance) quadsort_swap(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 9:
			if (abalance) quadsort_swap(array, swap, swap_size, quad1, cmp);
			fulcrum_partition(pta + 1, swap, (T*)nullptr, swap_size, quad2 + quad3, cmp);
			if (dbalance) quadsort_swap(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 12:
			fulcrum_partition(array, swap, (T*)nullptr, swap_size, half1, cmp);
			if (cbalance) quadsort_swap(ptb + 1, swap, swap_size, quad3, cmp);
			if (dbalance) quadsort_swap(ptc + 1, swap, swap_size, quad4, cmp);
			break;
		case 5:
		case 6:
		case 7:
		case 10:
		case 11:
		case 13:
		case 14:
		case 15:
#ifndef cmp
		quad_cache:
#endif
			if (asum)
			{
				if (abalance) quadsort_swap(array, swap, swap_size, quad1, cmp);
			}
			else fulcrum_partition(array, swap, (T*)nullptr, swap_size, quad1, cmp);
			if (bsum)
			{
				if (bbalance) quadsort_swap(pta + 1, swap, swap_size, quad2, cmp);
			}
			else fulcrum_partition(pta + 1, swap, (T*)nullptr, swap_size, quad2, cmp);
			if (csum)
			{
				if (cbalance) quadsort_swap(ptb + 1, swap, swap_size, quad3, cmp);
			}
			else fulcrum_partition(ptb + 1, swap, (T*)nullptr, swap_size, quad3, cmp);
			if (dsum)
			{
				if (dbalance) quadsort_swap(ptc + 1, swap, swap_size, quad4, cmp);
			}
			else fulcrum_partition(ptc + 1, swap, (T*)nullptr, swap_size, quad4, cmp);
			break;
	}

	if (cmp(*pta, *(pta + 1)) == true)
	{
		if (cmp(*ptc, *(ptc + 1)) == true)
		{
			if (cmp(*ptb, *(ptb + 1)) == true)
			{
				return;
			}
		}
		else
		{
			rotate_merge_block(array + half1, swap, swap_size, quad3, quad4, cmp);
		}
	}
	else
	{
		rotate_merge_block(array, swap, swap_size, quad1, quad2, cmp);

		if (cmp(*ptc, *(ptc + 1)) == false)
		{
			rotate_merge_block(array + half1, swap, swap_size, quad3, quad4, cmp);
		}
	}
	rotate_merge_block(array, swap, swap_size, half1, half2, cmp);
}

// The next 4 functions are used for pivot selection

template<typename Iterator, typename Compare>
Iterator crum_binary_median(Iterator pta, Iterator ptb, size_t len, Compare cmp)
{
	while (len /= 2)
	{
		if (cmp(*(pta + len), *(ptb + len)) == true) pta += len; else ptb += len;
	}
	return cmp(*pta, *ptb) == false ? pta : ptb;
}

template<typename T, typename Iterator, typename Compare>
Iterator crum_median_of_cbrt(Iterator array, T* swap, size_t swap_size, size_t nmemb, int *generic, Compare cmp)
{
	Iterator pta, piv;
	size_t cnt, cbrt, div;

	for (cbrt = 32 ; nmemb > cbrt * cbrt * cbrt && cbrt < swap_size ; cbrt *= 2) {}

	div = nmemb / cbrt;

	pta = array + nmemb - 1 - (size_t) &div / 64 % div;
	piv = array + cbrt;

	for (cnt = cbrt ; cnt ; cnt--)
	{
		swap[0] = *--piv; *piv = *pta; *pta = swap[0];

		pta -= div;
	}

	cbrt /= 2;

	quadsort_swap(piv, swap, swap_size, cbrt, cmp);
	quadsort_swap(piv + cbrt, swap, swap_size, cbrt, cmp);

	*generic = (cmp(*(piv + cbrt * 2 - 1), *piv) == true) & (cmp(*(piv + cbrt - 1), *piv) == true);

	return crum_binary_median(piv, piv + cbrt, cbrt, cmp);
}

template<typename Iterator, typename Compare>
size_t crum_median_of_three(Iterator array, size_t v0, size_t v1, size_t v2, Compare cmp)
{
	size_t v[3] = {v0, v1, v2};
	char x, y, z;

	x = cmp(*(array + v0), *(array + v1)) == false;
	y = cmp(*(array + v0), *(array + v2)) == false;
	z = cmp(*(array + v1), *(array + v2)) == false;

	return v[(x == y) + (y ^ z)];
}

template<typename Iterator, typename Compare>
Iterator crum_median_of_nine(Iterator array, size_t nmemb, Compare cmp)
{
	size_t x, y, z, div = nmemb / 16;

	x = crum_median_of_three(array, div * 2, div * 1, div * 4, cmp);
	y = crum_median_of_three(array, div * 8, div * 6, div * 10, cmp);
	z = crum_median_of_three(array, div * 14, div * 12, div * 15, cmp);

	return array + crum_median_of_three(array, x, y, z, cmp);
}

template<typename T, typename Iterator, typename Compare>
size_t fulcrum_default_partition(Iterator array, T* swap, Iterator ptx, T* piv, size_t swap_size, size_t nmemb, Compare cmp)
{
	size_t i, cnt, val, m = 0;
	Iterator ptl, ptr, pta, tpa;

	scandum_copy_range(T, swap, array, 32);
	scandum_copy_range(T, swap + 32, array + nmemb - 32, 32);

	ptl = array;
	ptr = array + nmemb - 1;

	pta = array + 32;
	tpa = array + nmemb - 33;

	cnt = nmemb / 16 - 4;

	while (1)
	{
		if (pta - ptl - m <= 48)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(*pta, *piv) == true; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
			}
		}
		if (pta - ptl - m >= 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(*tpa, *piv) == true; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
			}
		}
	}

	if (pta - ptl - m <= 48)
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(*pta, *piv) == true; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		}
	}
	else
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(*tpa, *piv) == true; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
		}
	}
	T* pta2 = swap;

	for (cnt = 16 ; cnt ; cnt--)
	{
		val = cmp(*pta2, *piv) == true; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
		val = cmp(*pta2, *piv) == true; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
		val = cmp(*pta2, *piv) == true; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
		val = cmp(*pta2, *piv) == true; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
	}
	return m;
}

// As per suggestion by Marshall Lochbaum to improve generic data handling by mimicking dual-pivot quicksort

template<typename T, typename Iterator, typename Compare>
size_t fulcrum_reverse_partition(Iterator array, T* swap, Iterator ptx, T* piv, size_t swap_size, size_t nmemb, Compare cmp)
{
	size_t i, cnt, val, m = 0;
	Iterator ptl, ptr, pta, tpa;

	scandum_copy_range(T, swap, array, 32);
	scandum_copy_range(T, swap + 32, array + nmemb - 32, 32);

	ptl = array;
	ptr = array + nmemb - 1;

	pta = array + 32;
	tpa = array + nmemb - 33;

	cnt = nmemb / 16 - 4;

	while (1)
	{
		if (pta - ptl - m <= 48)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(*piv, *pta) == false; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
			}
		}
		if (pta - ptl - m >= 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(*piv, *tpa) == false; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
			}
		}
	}

	if (pta - ptl - m <= 48)
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(*piv, *pta) == false; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		}
	}
	else
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(*piv, *tpa) == false; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
		}
	}
	T* pta2 = swap;

	for (cnt = 16 ; cnt ; cnt--)
	{
		val = cmp(*piv, *pta2) == false; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
		val = cmp(*piv, *pta2) == false; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
		val = cmp(*piv, *pta2) == false; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
		val = cmp(*piv, *pta2) == false; ptl[m] = ptr[m] = *pta2++; m += val; ptr--;
	}
	return m;
}

template<typename T, typename Iterator, typename Compare>
void fulcrum_partition(Iterator array, T* swap, T* max, size_t swap_size, size_t nmemb, Compare cmp)
{
	size_t a_size, s_size;
	Iterator ptp;
	T piv;
	int generic = 0;

	while (1)
	{
		if (nmemb <= 2048)
		{
			ptp = crum_median_of_nine(array, nmemb, cmp);
		}
		else
		{
			ptp = crum_median_of_cbrt(array, swap, swap_size, nmemb, &generic, cmp);

			if (generic) break;
		}
		piv = *ptp;

		if (max && cmp(*max, piv) == true)
		{
			a_size = fulcrum_reverse_partition(array, swap, array, &piv, swap_size, nmemb, cmp);
			s_size = nmemb - a_size;
			nmemb = a_size;

			if (s_size <= a_size / 32 || a_size <= CRUM_OUT) break;

			max = nullptr;
			continue;
		}
		*ptp = array[--nmemb];

		a_size = fulcrum_default_partition(array, swap, array, &piv, swap_size, nmemb, cmp);
		s_size = nmemb - a_size;

		ptp = array + a_size; array[nmemb] = *ptp; *ptp = piv;

		if (a_size <= s_size / 32 || s_size <= CRUM_OUT)
		{
			quadsort_swap(ptp + 1, swap, swap_size, s_size, cmp);
		}
		else
		{
			fulcrum_partition(ptp + 1, swap, max, swap_size, s_size, cmp);
		}
		nmemb = a_size;

		if (s_size <= a_size / 32 || a_size <= CRUM_OUT)
		{
			if (a_size <= CRUM_OUT) break;

			a_size = fulcrum_reverse_partition(array, swap, array, &piv, swap_size, nmemb, cmp);
			s_size = nmemb - a_size;
			nmemb = a_size;

			if (s_size <= a_size / 32 || a_size <= CRUM_OUT) break;

			max = nullptr;
			continue;
		}
		max = &*ptp;
	}
	quadsort_swap(array, swap, swap_size, nmemb, cmp);
}

template<typename T, typename Iterator, typename Compare>
void crumsort_swap(Iterator array, T* swap, size_t swap_size, size_t nmemb, Compare cmp)
{
	if (nmemb <= 256)
	{
		quadsort_swap(array, swap, swap_size, nmemb, cmp);
	}
	else
	{
		crum_analyze(array, swap, swap_size, nmemb, cmp);
	}
}

} // namespace scandum::detail

template<typename Iterator, typename Compare>
void crumsort(Iterator begin, const Iterator end, Compare cmp)
{
	typedef std::remove_reference_t<decltype(*begin)> T;

	size_t nmemb = static_cast<size_t>(end - begin);

	if (nmemb <= 256)
	{
		std::vector<T> swap(nmemb);
		detail::quadsort_swap(begin, swap.data(), nmemb, nmemb, cmp);
		return;
	}
#if CRUM_AUX
	size_t swap_size = CRUM_AUX;
#else
	size_t swap_size = 128;

	while (swap_size * swap_size <= nmemb)
	{
		swap_size *= 4;
	}
#endif
	std::vector<T> swap(swap_size);
	detail::crum_analyze(begin, swap.data(), swap_size, nmemb, cmp);
}

template<typename Iterator>
void crumsort(Iterator begin, Iterator end)
{
	typedef std::remove_reference_t<decltype(*begin)> T;
	return crumsort(begin, end, std::less<T>());
}

} // namespace scandum

#endif
