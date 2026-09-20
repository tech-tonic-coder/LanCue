# macOS / AppleClang compiler settings shared by every target.
# Kept separate from the top-level CMakeLists so a fourth platform later
# is one new file here, not an edit to call sites (per §4.4 of the roadmap).

add_compile_options(-Wall -Wextra -Werror)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
