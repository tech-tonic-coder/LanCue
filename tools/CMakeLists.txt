# Dev-only tools, not part of the shipped product and not tied to any
# specific roadmap phase — currently just lancue-debug-cli, a manual IPC
# testing aid (Phase 4, added to verify SetHotkey/RemoveHotkey without
# waiting for Phase 9's real settings UI). Not built by default the same
# way tests are, since it has no test-discovery entry point of its own;
# it's just another executable target under the same build.
add_subdirectory(lancue-debug-cli)
