# C59-5: QA report

Results per acceptance criterion of `spec.md`. Criteria not listed have
not been built yet (the design of C59-5 comes after the spikes, M-16).

## M-15 (2026-11-14): the XCFramework (AC-1 to AC-3)

Built on an M4 Pro, macOS 27.2, Xcode with the macOS 27.2 SDK; Homebrew
CMake and Ninja.

| AC | Result | How it was checked |
|---|---|---|
| AC-1 | Pass | `products/calc-u-59/frontends/swift/build_xcframework.sh` builds `build/xcframework-59/CalcU59.xcframework` from the root CMake project (`calcu59_core`, `calcu59_shared`, the library): slices `macos-arm64_x86_64`, `ios-arm64`, `ios-arm64_x86_64-simulator` (`lipo -archs`), headers under `calcu59/core/` and `calcu59/app/`. |
| AC-2 | Pass | The build is a step of the Xcode build: the aggregate target `CalcU59Core` runs the script and the app target depends on it, so it runs before Xcode processes the XCFramework (build log: `PhaseScriptExecution` before `ProcessXCFramework`). After `touch app/Bcd.cpp`, an `xcodebuild … build` of the app recompiled `Bcd.cpp` for the three slices, made the XCFramework again and relinked the app. Without CMake or Ninja the step stops the build with an `error:` that says the XCFramework cannot be built from the current sources. |
| AC-3 | Pass (build) | Minimum OS in the slices (`LC_BUILD_VERSION`): macOS 15.6, iOS 18.0, simulator 18.0; the Release Mac app is `x86_64 arm64` with `minos 15.6`; the app's deployment targets are unchanged (`IPHONEOS_DEPLOYMENT_TARGET = 18.0`, `MACOSX_DEPLOYMENT_TARGET = 15.6`). Built: macOS (Debug, and Release clean), iOS simulator, iOS device (`generic/platform=iOS`, unsigned). Not run on a device or an iOS 18 / macOS 15.6 system here: the app is not launched by an agent (`calc-u-59-swift` skill). |

Technical Requirements checked in the build: `CalcU59XcodeProjectCheck`
(Frequent, `frontends/swift/check_xcode_project.sh`): the project has no
folder or source of the library, the core or the app layer, links the
XCFramework, and the bridge includes only the XCFramework's headers. Xcode
compiles the five bridge `.mm` files and Swift, no `.cpp` (build logs of
all four builds above).
