# The library (ADR 0002): code used by more than one product. Empty until
# the first shared feature moves in (docs/migration-plan.md, Part C1); the
# products link calcu_library so that they build against it from the start.
add_library(calcu_library INTERFACE)
target_compile_features(calcu_library INTERFACE cxx_std_17)
