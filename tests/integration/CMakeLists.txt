# Cross-process integration tests: spins up the real lancue-core executable
# as a separate OS process and talks to it over the same QLocalSocket
# transport lancue-settings uses. An in-process fake server would only
# re-prove the framing/dispatch logic tests/core-lib already covers — this
# is specifically for the cross-process path.

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(integration-tests
    ipc_roundtrip_test.cpp
    settings_roundtrip_test.cpp
)

target_link_libraries(integration-tests
    PRIVATE
        core-lib
        Catch2::Catch2WithMain
)

# Generator-expression-only dependencies (used below in the source file via
# a compile definition) don't imply a build-order dependency on their own,
# so it's spelled out explicitly: integration-tests must not run before
# lancue-core exists to launch.
add_dependencies(integration-tests lancue-core)
target_compile_definitions(integration-tests
    PRIVATE
        LANCUE_CORE_EXECUTABLE="$<TARGET_FILE:lancue-core>"
)

include(CTest)
include(Catch)
catch_discover_tests(integration-tests)
