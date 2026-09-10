# Boost.int128
Portable implementations of signed and unsigned 128-bit integers

|                  | Master                                                                                                                                                            |   Develop   |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------|
| Drone            | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/int256/status.svg?ref=refs/heads/master)](https://drone.cpp.al/cppalliance/int256)                 | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/int256/status.svg?ref=refs/heads/develop)](https://drone.cpp.al/cppalliance/int256) |
| Github Actions   | [![CI](https://github.com/cppalliance/int256/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/cppalliance/int256/actions/workflows/ci.yml) | [![CI](https://github.com/cppalliance/int256/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/cppalliance/int256/actions/workflows/ci.yml)
| Codecov          | [![codecov](https://codecov.io/gh/cppalliance/int256/branch/master/graph/badge.svg?token=KWY7M64xhh)](https://codecov.io/gh/cppalliance/int256) | [![codecov](https://codecov.io/gh/cppalliance/int256/graph/badge.svg?token=drvY8nnV5S)](https://codecov.io/gh/cppalliance/int256) |
---

# How To Use The Library

This library is header only and requires C++14 or newer.
With C++20 and greater you can instead `import boost.int256`.

You can also try the library out on [Compiler Explorer](https://godbolt.org/z/3q4zce41d).

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
* Intel OneAPI DPC++

# Synopsis

Int256 provides 2 types:

```cpp
namespace boost {
namespace int256 {

struct uint256;
struct int256;

} //namespace int256
} //namespace boost
```

These types operate like built-in integer types.
They have their own implementations of the Standard-Library functions
(e.g., like those found in `<limits>`, `<iostream>`, `<bit>` etc.).

The entire library can be conveniently included with `#include <boost/int256.hpp>`

# Full Documentation

The complete documentation can be found at: https://develop.int256.cpp.al

# Help and feedback

* **[GitHub Issues](https://github.com/cppalliance/int256/issues)** for bug reports. Search before opening a new one.
* **[GitHub Discussions](https://github.com/cppalliance/int256/discussions)** for questions, design ideas, and general conversation about the library.
* **[Boost mailing list](http://lists.boost.org/mailman/listinfo.cgi/boost-users)** for general Boost development. Use the `[int256]` tag in the subject line.
* **CppLang Slack** for real-time chat. [Request an invite](https://cppalliance.org/slack/), then join the `#boost` channel.
