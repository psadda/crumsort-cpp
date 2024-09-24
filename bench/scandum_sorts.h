#ifndef SCANDUM_SORTS_H
#define SCANDUM_SORTS_H

#include <stddef.h> // for size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef int CMPFUNC(const void* a, const void* b);

void blitsort(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);
void blitsort_prim(void* array, size_t nmemb, size_t size);

void crumsort(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);
void crumsort_prim(void* array, size_t nmemb, size_t size);

void fluxsort(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);
void fluxsort_prim(void* array, size_t nmemb, size_t size);
void fluxsort_size(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);

void gridsort(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);

void quadsort(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);
void quadsort_prim(void* array, size_t nmemb, size_t size);
void quadsort_size(void* array, size_t nmemb, size_t size, CMPFUNC* cmp);

// octosort, piposort, skipsort, and wolfsort are not included because they
// require patches to compile on clang or msvc (or any other compiler where
// sizeof(double) == sizeof(long double))

// This function also requires sizeof(double) == sizeof(long double) and isn't
// actually compiled on clang or msvc, but gridsort requires this forward
// declaration regardless of platform
void quadsort_swap128(void* array, void* swap, size_t swap_size, size_t nmemb, CMPFUNC* cmp);

#ifdef __cplusplus
}
#endif

#endif
