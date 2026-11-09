# The Swift frontend links an XCFramework built by CMake

Status: accepted (2026-11-09)

The root CMake project (ADR 0003) builds the library, the core and the app
layer of a product as an XCFramework for macOS, iOS devices and the iOS
simulator; the Xcode project links it and builds only Swift and the
bridge. Today Xcode compiles Calc-U-59's `Core/` and `Shared/` itself from
synchronized folders, a second build system whose compiler flags can drift
from CMake's. The cost is a build step before Xcode (a CMake preset or a
script phase).

## Considered options

- Xcode keeps compiling the C++ sources: faster to iterate inside Xcode,
  but every flag and source list is maintained twice.
