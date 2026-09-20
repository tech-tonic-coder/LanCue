# Linux / GCC-Clang compiler settings shared by every target.
# Kept separate from the top-level CMakeLists so a fourth platform later
# is one new file here, not an edit to call sites (per §4.4 of the roadmap).

add_compile_options(-Wall -Wextra -Werror)
