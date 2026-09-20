# Windows / MSVC compiler settings shared by every target.
# Kept separate from the top-level CMakeLists so a fourth platform later
# is one new file here, not an edit to call sites (per §4.4 of the roadmap).

if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    # Qt headers still trip a few informational MSVC warnings we don't own; silence
    # only these codes rather than lowering /W4 globally.
    add_compile_options(/wd4251 /wd4275)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)

    # RelWithDebInfo is the recommended local dev config for anything linking
    # Qt (see cmake/qt-deploy-platform-plugin.cmake / roadmap Learnings log,
    # Phase 0): this vcpkg qtbase build only ships a Release-mode platform
    # plugin, so a plain Debug lancue-settings can never load it. RelWithDebInfo
    # keeps the Release CRT/ABI (so the plugin loads) while still producing
    # full PDBs. Turning optimization back off for *our own* translation units
    # only (not Qt's prebuilt binaries — those aren't recompiled here) keeps
    # breakpoint/step/inspect debugging as close to a real Debug build as
    # possible without touching ABI compatibility.
    add_compile_options("$<$<CONFIG:RelWithDebInfo>:/Od>")
endif()
