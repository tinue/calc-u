# The emulation core and the frontend-neutral Shared/ layer as two static
# libraries, for every CMake build in the repository:
#
#   calcu_core    Core/  -- the hardware (CPUs, machines, cards, loaders, presets)
#   calcu_shared  Shared/ -- everything above Core that isn't drawing
#                 (docs/developer/Shared-Architecture.md); links calcu_core
#
# Included once, by the product's CMakeLists.txt (a subdirectory of the
# repository's root CMake project, ADR 0003).
#
# The source lists are globbed: every core/**/*.cpp except core/tests/ and
# every app/**/*.cpp except app/tests/.
#
# calcu_core is compiled at -O1 in Debug unless
# CALCU_TESTS_DEBUG is ON (calcu_debug_o1, the root CMakeLists.txt).

get_filename_component(CALCU_ROOT ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(CALCU_CORE_ROOT ${CALCU_ROOT}/core)
set(CALCU_SHARED_ROOT ${CALCU_ROOT}/app)

# ---- libsharpdx: every program loader needs it (Core/Basic/vendor/sharpdx/README.md)
#  - macOS: libsharpdx.a is committed (tools/refresh_sharpdx.sh).
#  - Linux/Windows: fetched by tools/fetch_sharpdx.sh into libsharpdx-linux.a
#    / sharpdx.lib -- different names, so a fetch never dirties the
#    committed macOS binary.
#  - Emscripten: libsharpdx-wasm.a (tools/build_sharpdx_wasm.sh).
if(EMSCRIPTEN)
  set(SHARPDX_LIB ${CALCU_CORE_ROOT}/Basic/vendor/sharpdx/libsharpdx-wasm.a)
elseif(WIN32)
  set(SHARPDX_LIB ${CALCU_CORE_ROOT}/Basic/vendor/sharpdx/sharpdx.lib)
elseif(APPLE)
  set(SHARPDX_LIB ${CALCU_CORE_ROOT}/Basic/vendor/sharpdx/libsharpdx.a)
else()
  set(SHARPDX_LIB ${CALCU_CORE_ROOT}/Basic/vendor/sharpdx/libsharpdx-linux.a)
endif()
if(NOT EXISTS ${SHARPDX_LIB})
  message(FATAL_ERROR "${SHARPDX_LIB} is missing -- run tools/fetch_sharpdx.sh (Linux/Windows), "
                      "tools/refresh_sharpdx.sh (macOS) or tools/build_sharpdx_wasm.sh (Emscripten)")
endif()

add_library(calcu_sharpdx INTERFACE)
target_link_libraries(calcu_sharpdx INTERFACE ${SHARPDX_LIB})
# The vendored/fetched libs are sharpdx's serial-free build, so there's no
# libudev (Linux), IOKit (macOS) or serialport import lib (Windows) to link
# -- only Rust std's own Windows system libs, below.
if(WIN32)
  # `cargo build` links the Windows system import libs Rust's std needs
  # (sockets, CSPRNG seeding, futex-based sync, ...) into sde.exe
  # automatically; consuming the raw sharpdx.lib from CMake doesn't, so
  # without this the link fails with unresolved externals such as
  # __imp_freeaddrinfo (ws2_32). The authoritative list is the one rustc
  # reported for that exact build: SharpDataExchange's release packaging
  # captures `rustc --print=native-static-libs` into
  # native-libs-windows.txt and tools/fetch_sharpdx.sh vendors it next to
  # sharpdx.lib. Only a hand-placed sharpdx.lib without that file falls
  # back to the hardcoded guess.
  #
  # On *-pc-windows-msvc rustc's note is MSVC linker syntax (bare
  # `name.lib` names and the odd `/defaultlib:x`), not `-lname`, so it goes
  # in as raw link options: target_link_libraries would look for a library
  # literally named "/defaultlib:msvcrt" and fail.
  set(SHARPDX_NATIVE_LIBS_FILE ${CALCU_CORE_ROOT}/Basic/vendor/sharpdx/native-libs-windows.txt)
  if(EXISTS ${SHARPDX_NATIVE_LIBS_FILE})
    file(READ ${SHARPDX_NATIVE_LIBS_FILE} SHARPDX_NATIVE_LIBS_RAW)
    string(STRIP "${SHARPDX_NATIVE_LIBS_RAW}" SHARPDX_NATIVE_LIBS_RAW)
    separate_arguments(SHARPDX_NATIVE_LINK_OPTS UNIX_COMMAND "${SHARPDX_NATIVE_LIBS_RAW}")
    message(STATUS "sharpdx: linking native libs from native-libs-windows.txt: ${SHARPDX_NATIVE_LINK_OPTS}")
    target_link_options(calcu_sharpdx INTERFACE ${SHARPDX_NATIVE_LINK_OPTS})
  else()
    set(SHARPDX_NATIVE_LIBS_FALLBACK ws2_32 userenv advapi32 bcrypt ntdll synchronization)
    message(STATUS "sharpdx: native-libs-windows.txt not found, using hardcoded fallback: ${SHARPDX_NATIVE_LIBS_FALLBACK}")
    target_link_libraries(calcu_sharpdx INTERFACE ${SHARPDX_NATIVE_LIBS_FALLBACK})
  endif()
endif()

# ---- calcu_core
file(GLOB_RECURSE CALCU_CORE_SOURCES CONFIGURE_DEPENDS ${CALCU_CORE_ROOT}/*.cpp)
list(FILTER CALCU_CORE_SOURCES EXCLUDE REGEX "^${CALCU_CORE_ROOT}/tests/")

# calcu_add_core(<name>): a static library of every core/ source (calcu_core;
# the probes' copy with PC1600_POWER_PROBE went with the probes in M-51).
function(calcu_add_core name)
  add_library(${name} STATIC ${CALCU_CORE_SOURCES})
  target_include_directories(${name} PUBLIC ${CALCU_CORE_ROOT})
  target_compile_features(${name} PUBLIC cxx_std_20)
  target_link_libraries(${name} PRIVATE calcu_warnings)
  # The sources are UTF-8 without a BOM (U'á' literals, PC1600TypedInput.hpp);
  # MSVC would read them in the ANSI codepage (C2015). Targets linking Qt got
  # /utf-8 from Qt6::Platform; core/ and app/ don't link Qt, so set it here --
  # PUBLIC, since everything that includes core/'s headers needs it too.
  if(MSVC)
    target_compile_options(${name} PUBLIC /utf-8)
  endif()
  target_link_libraries(${name} PUBLIC calcu_sharpdx)
  # The library (the root project's calcu_library): the machine interface,
  # the settings file (LIB-9). Products do not configure on their own.
  target_link_libraries(${name} PUBLIC calcu_library)
  calcu_debug_o1(${name})
endfunction()
calcu_add_core(calcu_core)

# ---- calcu_shared
# Shared/ is C++20 and standard-library-only: no Qt, Cocoa or Emscripten
# headers, and single-threaded (docs/developer/Shared-Architecture.md).
# Headers include their Core dependencies relative to Core/ ("PC1600/...")
# and each other as "Shared/...".
file(GLOB_RECURSE CALCU_SHARED_SOURCES CONFIGURE_DEPENDS ${CALCU_SHARED_ROOT}/*.cpp)
list(FILTER CALCU_SHARED_SOURCES EXCLUDE REGEX "^${CALCU_SHARED_ROOT}/tests/")
# The harness product entry (LIB-11, M-28) is a target of its own:
# calcu1600_harness in CMakeLists.txt, not in the Qt6 and Web builds.
list(FILTER CALCU_SHARED_SOURCES EXCLUDE REGEX "/app/HarnessProduct\\.cpp$")
# The DAP's layer (C1600-3 §3.1): every tree but Emscripten -- the Web has
# no debugger client.
if(EMSCRIPTEN)
  list(FILTER CALCU_SHARED_SOURCES EXCLUDE REGEX "/app/dap/")
endif()
add_library(calcu_shared STATIC ${CALCU_SHARED_SOURCES})
target_include_directories(calcu_shared PUBLIC ${CALCU_ROOT})
target_link_libraries(calcu_shared PRIVATE calcu_warnings)
target_link_libraries(calcu_shared PUBLIC calcu_core)
