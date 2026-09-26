# eolib-cpp

Core C++ library for writing Endless Online applications.

## Features

Read and write the following EO data structures:

- Client packets
- Server packets
- Endless Map Files (EMF)
- Client pub files (items, npcs, spells, classes)
- Server pub files (talk, shops, drops, inns, skillmasters)

Utilities:

- Data reader/writer
- Number/string encoding
- Data encryption
- Packet sequencing

## Requirements

- A C++17 compiler (GCC 8+, Clang 7+, AppleClang 11+, MSVC 2019+)
- CMake 3.16+

## Building from source

```sh
git clone --recurse-submodules https://github.com/ethanmoffat/eolib-cpp.git
cd eolib-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```
