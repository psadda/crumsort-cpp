Crumsort and Quadsort in C++
============================

This is a C99 to C++17 port of Igor van den Hoven's crumsort and quadsort.

Porting crumsort and quadsort to C++ is not as trivial as one might expect. The original crumsort and quadsort have many C-isms that don't map well to modern C++, preventing them from being used as drop in replacements for `std::sort`:

- They take raw pointers as input, not random access iterators. That means they only work for arrays of contiguous memory, like `std::vector`, and not on discontiguous containers, like `std::deque`.
- They use C99 variable length arrays, which are not part of the C++ standard. Some C++ compilers support VLAs as a language extension, but others (MSVC) do not.
- They assume that that the sorted type is [trivial](https://en.cppreference.com/w/cpp/named_req/TrivialType).

I will work on fixing these limitations as time allows.

See the original C implementations at [scandum/crumsort](https://github.com/scandum/crumsort) and [scandum/quadsort](https://github.com/scandum/quadsort) for detailed descriptions of the algorithms and their properties.

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
- [ ] Re-enable optimizations for primitive types

License
-------

[Unlicense](https://unlicense.org/)
