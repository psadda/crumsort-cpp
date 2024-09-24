#include "scandum_sorts.h"

#include "blitsort.h"
#include "crumsort.h"
#include "fluxsort.h"
#include "gridsort.h"
#include "quadsort.h"

// Dummy function to allow gridsort to compile
void quadsort_swap128(void* array, void* swap, size_t swap_size, size_t nmemb, CMPFUNC* cmp) {}
