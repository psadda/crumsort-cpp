Crumsort in C++
===============

This is a C to C++ port of Igor van den Hoven's crumsort.

Porting crumsort to C++ is not as trivial as one might expect. The original crumsort has many C-isms that don't map well to modern C++.

This port is still a work in progress. As of now, it provides a type safe interface and accepts a compare functor, but there are still some issues that have to be fixed before it can be used as a drop in replacement for `std::sort`:

- It uses pointers, not random access iterators. This means that crumsort-cpp only works on arrays of contiguous memory, like `std::vector`, but not on discontiguous containers, like `std::deque`.
- It assumes that that the sorted type is [trivial](https://en.cppreference.com/w/cpp/named_req/TrivialType).
- It uses variable length arrays, which some C++ compilers do not support (MSVC) since VLAs are not part of the C++ standard.

I will work on fixing these limitations as time allows.

See the original C implementation at [scandum/crumsort](https://github.com/scandum/crumsort) for a detailed description of the algorithm and its properties.

Progress
--------

- [x] Typesafe interface (no `void*`)
- [x] Accept functors as the predicate
- [x] Remove use of C99 VLAs
- [ ] Accept any random access iterator (not just raw pointers)
- [ ] Support nontrivial types (requires removing uses of `memcpy`, `memmove`, `memset`)
- [ ] Support non-default-constructible types
- [ ] Support move only types

Example
-------

```cpp
#include "crumsort.hpp"

#include <vector>
#include <functional>
#include <iostream>

int main(int argc, char** argv) {
    std::vector<int> list = { 1, 4, 0, 7, 1, 12 };

    scandum::crumsort(list.data(), list.data() + list.size(), std::greater<int>());

    for (int value : list) {
        std::cout << value << '\n';
    }

    return 0;
}
```

License
-------

[Unlicense](https://unlicense.org/)
