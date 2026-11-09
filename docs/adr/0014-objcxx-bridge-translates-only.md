# The Swift bridge is ObjC++ and only translates

Status: accepted (2026-11-09)

The Swift frontend reaches the app session through an ObjC++ bridge that
translates types and holds no logic, as Calc-U-59's `Bridge/` does today.
The bridge wraps the app session's frontend interface, so the same pattern
serves a later Swift frontend for Calc-U-1600 (Briefing §1.4). Swift/C++
interop (Swift 5.9+) was rejected for now: it supports only part of C++
(some templates, coroutine and move-only types), and the app session uses
C++20 coroutines (ADR 0009). It can be revisited without touching the app
layer.
