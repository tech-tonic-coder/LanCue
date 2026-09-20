add_executable(lancue-debug-cli
    main.cpp
)

target_link_libraries(lancue-debug-cli
    PRIVATE
        core-lib
)
