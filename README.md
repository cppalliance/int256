# Boost.Int256
Portable implementation of unsigned and signed 256-bit integers

|                  | Master                                                                                                                                                            |   Develop   |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------|
| Drone            | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/int256/status.svg?ref=refs/heads/master)](https://drone.cpp.al/cppalliance/int256)                 | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/int256/status.svg?ref=refs/heads/develop)](https://drone.cpp.al/cppalliance/int256) |
| Github Actions   | [![CI](https://github.com/cppalliance/int256/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/cppalliance/int256/actions/workflows/ci.yml) | [![CI](https://github.com/cppalliance/int256/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/cppalliance/int256/actions/workflows/ci.yml)
| Codecov          | [![codecov](https://codecov.io/gh/cppalliance/int256/branch/master/graph/badge.svg?token=BlsDTjdchH)](https://codecov.io/gh/cppalliance/int256) | [![codecov](https://codecov.io/gh/cppalliance/int256/graph/badge.svg?token=BlsDTjdchH)](https://codecov.io/gh/cppalliance/int256) |
---

# How To Use The Library

This library is header only and requires C++14 or newer. It contains no other dependencies.
Simply `#include` it and use it.
The library is also available as a single header [here](https://github.com/cppalliance/int256/blob/develop/extra/single_include/boost/int256.hpp), which CI regenerates from `include/` on every change.
With C++20 and greater you can instead `import boost.int256`.

## CMake

```sh
git clone https://github.com/cppalliance/int256
cd int256
mkdir build && cd build
cmake .. OR cmake .. -DCMAKE_INSTALL_PREFIX=/your/custom/path
cmake --build . --target install
```

then you can use `find_package(boost_int256 REQUIRED)`

## vcpkg

```sh
git clone https://github.com/cppalliance/int256
cd int256
vcpkg install int256 --overlay-ports=ports/int256
```

# Supported Platforms

Boost.Int256 is tested natively on Ubuntu (x86_64, x86_32, s390x, aarch64, ARM32v7), macOS (x86_64, and Apple Silicon), and Windows (x86_64, x86_32, and ARM64);
as well as emulated PPC64LE using QEMU with the following compilers:

* GCC 5 and later
* Clang 5 and later
* Visual Studio 2017 (14.1) and later
* Intel OneAPI DPC++ 2024.2 and later
* NVCC 12.8 and later

# Synopsis

Int256 provides two types:

```cpp
namespace boost {
namespace int256 {

struct uint256;
struct int256;

} //namespace int256
} //namespace boost
```

`int256` shares `uint256`'s layout (`std::uint64_t words[4]`, interpreted as
two's complement), header set, and macro names, so a value can be converted
between the two types with a plain bit copy.

Both types operate like a built-in integer type.
Each has its own implementations of the Standard-Library functions
(e.g., like those found in `<limits>`, `<iostream>`, `<bit>` etc.).

The entire library can be conveniently included with `#include <boost/int256.hpp>`

# Full Documentation

The complete documentation can be found at: https://develop.int256.cpp.al

# Help and feedback

* **[GitHub Issues](https://github.com/cppalliance/int256/issues)** for bug reports. Search before opening a new one.
* **[GitHub Discussions](https://github.com/cppalliance/int256/discussions)** for questions, design ideas, and general conversation about the library.
* **[Boost mailing list](http://lists.boost.org/mailman/listinfo.cgi/boost-users)** for general Boost development. Use the `[int256]` tag in the subject line.
* **CppLang Slack** for real-time chat. [Request an invite](https://cppalliance.org/slack/), then join the `#boost` channel.
