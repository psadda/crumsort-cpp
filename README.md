Crumsort and Quadsort in C++
============================

This is a C99 to C++17 port of Igor van den Hoven's crumsort and quadsort.

Porting crumsort and quadsort to C++ is not as trivial as one might expect. The original implementation has many C-isms that don't map well to modern C++ and prevent it from being used as a drop in replacement for `std::sort`:

- It uses pointers, not random access iterators. This means that crumsort-cpp only works on arrays of contiguous memory, like `std::vector`, and not on discontiguous containers, like `std::deque`.
- It assumes that that the sorted type is [trivial](https://en.cppreference.com/w/cpp/named_req/TrivialType).
- It uses variable length arrays, which some C++ compilers do not support (MSVC) since VLAs are not part of the C++ standard.

I will work on fixing these limitations as time allows.

See the original C implementation at [scandum/crumsort](https://github.com/scandum/crumsort) for a detailed description of the algorithm and its properties.

Example
-------

```cpp
#include "crumsort.hpp"

#include <vector>
#include <functional>
#include <iostream>

int main(int argc, char** argv) {
    std::vector<int> list = { 1, 4, 0, 7, 1, 12 };

    scandum::crumsort(list.begin(), list.end(), std::less<int>());

    for (int value : list) {
        std::cout << value << '\n';
    }

    return 0;
}
```

Progress
--------

- [x] Typesafe interface (no `void*`)
- [x] Accept functors as the predicate
- [x] Remove use of C99 VLAs
- [x] Accept any random access iterator (not just raw pointers)
- [x] Support types that are not trivially copyable (requires removing uses of `memcpy`, `memmove`, `memset`)
- [x] Support types that do not a have a trivial default constructor
- [ ] Support types that do not have *any* default constructor
- [ ] Support move only types

License
-------

[Unlicense](https://unlicense.org/)
