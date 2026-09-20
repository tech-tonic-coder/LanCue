# lancue-cli: terminal-only conversion frontend (§5 Phase 6, requested by
# Mahdi 2026-08-07). Links core-lib directly rather than talking to a
# running lancue-core over IPC — the open design decision Phase 6's own
# roadmap text flagged as unresolved. Decided here, standalone-linking,
# because:
#   - core-lib's conversion engine (conversion/layout_char_maps.cpp,
#     conversion_engine.cpp) is pure data + logic with zero dependency on
#     anything IPC/daemon-related — reaching it through a socket round
#     trip would be solving a problem that doesn't exist for this tool.
#   - settings::SettingsManager (also core-lib, see settings_manager.h)
#     already reads/writes its JSON file directly with no daemon
#     involved, so even a *future* lancue-cli feature needing configured
#     layoutConversionPairs could read the same on-disk settings file
#     directly — the single source of truth is the file, not whichever
#     process has it open — without gaining an IPC dependency either.
#   - A terminal tool that only works while a background GUI daemon
#     happens to be running (and fails confusingly in a script/CI/SSH
#     context otherwise) is a worse fit for "terminal command that runs a
#     conversion without any GUI... interaction" than one with no runtime
#     dependency on lancue-core at all.
# This build does not use SettingsManager yet (see main.cpp — --from/--to
# are required, no auto-detect-via-configured-pairs mode exists in this
# phase), so there is no state duplication to worry about in practice
# yet either.

find_package(Qt6 CONFIG REQUIRED COMPONENTS Core)

add_executable(lancue-cli
    src/main.cpp
)

target_link_libraries(lancue-cli
    PRIVATE
        core-lib
        Qt6::Core
)

if(WIN32)
    # For CommandLineToArgvW (main.cpp's parseCommandLineArgs()) — also
    # pragma-linked directly in main.cpp for MSVC specifically, but listed
    # here too so the dependency is visible in the build definition itself
    # and doesn't silently rely on a compiler-specific pragma alone.
    target_link_libraries(lancue-cli PRIVATE Shell32)
endif()
