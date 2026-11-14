cmake_minimum_required(VERSION 3.21)

# The one CMake project of the repository (ADR 0003): the library, the
# cores, app layers, tests, tools and Qt6 frontends of every product. Local
# builds, IDEs, the harness and CI configure this file; the product
# directories are its subdirectories and do not configure on their own.
#
#   cmake -S . -B build/all -G Ninja                         # everything, Qt6 when found
#   cmake -S . -B build/tests -G Ninja -DCALCU_FRONTEND_QT=OFF  # cores and tests, no Qt
#   ctest --test-dir build/tests/products/calc-u-59          # one product's tests
#   emcmake cmake -S . -B build/web -G Ninja -DCMAKE_BUILD_TYPE=Release  # the Web engines
#
# Options:
#   CALCU_PRODUCT_59    Calc-U-59 (products/calc-u-59/)
#   CALCU_PRODUCT_1600  Calc-U-1600 (products/calc-u-1600/)
#   CALCU_FRONTEND_QT   the Qt6 desktop apps; without it nothing needs Qt
#   CALCU_TESTS         the products' test suites; the release packaging in
#                       CI turns them off (build/release, the Qt6 apps only)
#
# Under the Emscripten toolchain (emcmake) each product defines only its
# Web engine (calcu59_web, calcu_web) and Calc-U-1600's wasm_smoke check;
# frontends/web/build.sh of each product builds it in build/web and puts
# dist/ together. Not here: the Swift frontend (Xcode compiles core/ and
# app/ itself; ADR 0013, M-15).

# One minimum for the whole tree, before project(): Calc-U-1600 needs 15.8
# (its vendored libsharpdx is built for it); the Calc-U-59 Qt build on the
# Mac is for development only (its Mac product is the Swift app).
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.8" CACHE STRING "Minimum macOS version")

if(APPLE)
  # Calc-U-1600's Qt6 app has Objective-C++ sources.
  project(Calc-U LANGUAGES CXX OBJCXX)
else()
  project(Calc-U LANGUAGES CXX)
endif()

option(CALCU_PRODUCT_59 "Build Calc-U-59" ON)
option(CALCU_PRODUCT_1600 "Build Calc-U-1600" ON)
option(CALCU_FRONTEND_QT "Build the Qt6 desktop apps" ON)
option(CALCU_TESTS "Build the products' test suites" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

if(CALCU_FRONTEND_QT AND APPLE AND NOT DEFINED CMAKE_PREFIX_PATH)
  # Homebrew's Qt, for both products' frontends.
  foreach(prefix /opt/homebrew/opt/qt /usr/local/opt/qt)
    if(EXISTS ${prefix})
      list(APPEND CMAKE_PREFIX_PATH ${prefix})
      break()
    endif()
  endforeach()
endif()

add_subdirectory(library)

if(CALCU_PRODUCT_59)
  add_subdirectory(products/calc-u-59)
endif()
if(CALCU_PRODUCT_1600)
  add_subdirectory(products/calc-u-1600)
endif()
