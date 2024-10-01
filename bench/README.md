Benchmarks
==========

The results shown here were obtained from a release build compiled with ClangCL with optimization flags `/O3` and `/DNDEBUG`.

## Results ##

#### Random ####

*The input array is composed of unsorted random integers.*

<img alt="graph of benchmark results on unsorted random integers" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/random%2010000.png" width="480">

#### Random High Bits ####

*The input array is composed of unsorted random integers, but the randomness is in the high (most significant) bits of the integers rather than the low (least significant) bits.*

<img alt="graph of benchmark results on unsorted random strings" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/random%20high%20bits%2010000.png" width="480">

#### Random Half ####

*Half of the input array is already in sorted order; the other half is unsorted.*

<img alt="graph of benchmark results on half unsorted, half sorted integers" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/random%20half%2010000.png" width="480">

#### Ascending ####

*The input array is already in sorted order.*

<img alt="graph of benchmark results on sorted, ascending integers" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/ascending%2010000.png" width="480">

#### Descending ####

*The input array is already sorted in reverse order.*

<img alt="graph of benchmark results on sorted, descending integers" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/descending%2010000.png" width="480">

#### Ascending Saw ####

*The input array is composed of alternating runs of sorted and unsorted integers.*

<img alt="graph of benchmark results on alternating runs of sorted and unsorted integers" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/ascending%2010000.png" width="480">

#### Ascending Tiles ####

*The input array is divided into two chunks, each of which are internally sorted.*

<img alt="graph of benchmark results on ascending tiles" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/ascending%20tiles%2010000.png" width="480">

#### Pipe Organ ####

*The first half of the input array is sorted in ascending order and the second half is sorted in descending order.*

<img alt="graph of benchmark results on a pipe organ array" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/pipe%20organ%2010000.png" width="480">

#### Bit Reversal ####

*The input array is composed of sequential integers that have been bit-reversed, then bit-rotated.*

<img alt="graph of benchmark results on a pipe organ array" src="https://github.com/psadda/crumsort-cpp/blob/main/bench/results/bit%20reversal%2010000.png" width="480">

## A Deeper Look at the Candidates ##

### General Purpose Sorts: `crumsort`, `quadsort`, `pdqsort`, `timsort` ###

These functions accept any comparison predicate (as long as that) and can sort any type of movable data. `pdsort` and the C++ version of `crumsort` can be used as drop in replacements for `std::sort`. `timsort` and the C++ version of `quadsort` can be used as drop in replacments for `std::stable_sort`.

The C versions of `crumsort` and `quadsort` can be compiled with a `cmp` macro that replaces the general purpose predicate with a hardcoded greater-than or less-than predicate for numeric types, slightly improving performance. They were _**not**_ compiled with `cmp` in this benchmark.

### Numeric-Only Sorts: `ska_sort`, `rhsort`, `x86-simd-sort`, `vqsort` ###

These sorts incoporate a radix sort or counting sort. That makes them very fast for sorting numeric data, but it also comes with some limitations:

* They are _**not**_ general purpose sorts: the key has to be numeric, and the sorting predicate has to be a simple numeric less-than or greater-than. (`ska_sort` is slightly more flexible than the others — it also accepts arrays of numbers as the key. This allows you to use strings as the sorting key, but thanks to the limitation on the predicate you wouldn't be able to do, say, a case-insensitive string sort without first transforming the sorting key.)
* Their performance is sensitive to the bit-length of the key. They may be limited to keys of a particular size.
* `x86-simd-sort` and `vqsort` are SIMD accelerated and will only work on CPUs that implement one of their supported instruction sets.
* All of the above mean that these functions are _**not**_ drop in replacements for `std::sort`, although they could be used to optimize  specializations of `std::sort`.

| Algorithm       | Time (Avg) | Space (Avg) | Stable | Key Type                    | Key Size               | Portability        |
| --------------- | ---------- | ----------- | -------| --------------------------- | ---------------------- | ------------------ |
| `ska_sort`      | O(n)       | O(n)        | no     | numeric or array of numeric | any                    | any                |
| `rhsort`        | O(n)       | O(n)        | yes    | numeric only                | 32 bit only            | any                |
| `x86-simd-sort` | O(n)       | O(1)/O(n)†  | no     | numeric only                | 32 or 64 bit           | AVX2, AVX512       |
| `vqsort`        | O(n)       | O(n)        | no     | numeric only                | 16, 32, 64, or 128 bit | AVX2, AVX512, NEON |

† `x86-simd-sort` uses O(1) space when sorting numeric data and O(n) space when sorting arbitrary data with a numeric key.

This benchmark uses the AVX2 flavors of `x86-simd-sort` and `vqsort`.

## Concluding Remarks ##

* C++ `crumsort` and `quadsort` are competetive across the entire suite of tests — almost always best or second best. Performance is generally on par with that of the C versions.
* `std::sort` and `std::stable_sort` (at least the Microsoft implementations tested in this benchmark) have generally okay performance. If sorting isn't a bottleneck, it's reasonable to stick with the standard library sorts to avoid introducing a new dependency.
* `qsort`, on the other hand, is absolutely terrible. But this may be Microsoft specific, since C and the C standard library are second class citizens in MSVC space.
* `pdqsort` is pretty good across the board. It's a step ahead of `timsort`, which struggles with some data patterns. It's also a much simpler algorithm than `crumsort`, so it's a good option for those who are looking to balance runtime performance with binary size and code complexity.
* If you only need to sort numeric data, `rhsort` is very fast. `x86-simd-sort` and `vqsort` are sometimes faster if you can rely on compatible vector extensions being present. However, the more general purpose `crumsort` and `quadsort` still perform better in several benchmarks.
