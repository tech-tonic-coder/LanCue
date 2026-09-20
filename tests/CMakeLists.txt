# Test framework decision (Phase 1, see roadmap Learnings log §6.2):
# Catch2 via vcpkg, wired into ctest through catch_discover_tests in each
# subdirectory below.

add_subdirectory(core-lib)
add_subdirectory(integration)
