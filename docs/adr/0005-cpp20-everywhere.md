# C++20 for the whole repository

Status: accepted (2026-11-09)

The library, both cores, the app layers and the C++/ObjC++ parts of every
frontend use C++20 (ObjC++ in the Swift bridge as gnu++20). Calc-U-1600
already uses C++20; Calc-U-59 is raised from C++17, because the library
must compile in both products and Briefing §8 allows raising a language
version (never lowering), each raise recorded as an ADR. Xcode, Qt 6.4 and
later, MSVC and Emscripten support C++20.
