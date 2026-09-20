# LanCue

Cross-platform keyboard-layout toast/indicator/converter utility. See
`LanCue-Roadmap.md` for the full architecture, coding rules, and phase plan —
that file is the source of truth; this README is just "how do I build it."

## Phase 1 status

Phase 0's scaffolding now carries real content: `lancue-core` and
`lancue-settings` exchange a message over IPC (a `QLocalSocket`/
`QLocalServer` pair, length-prefixed JSON envelopes), `lancue-core` enforces
single-instance startup via `QLockFile` and runs headless on Windows
(`WIN32_EXECUTABLE`), and `core-lib` gained an in-process `EventBus` that the
IPC layer already publishes onto. Catch2 is wired in as the test framework
(`tests/core-lib` for unit tests, `tests/integration` for a real
cross-process round trip against the actual `lancue-core` binary). No
daemon feature (toast, layout watcher, etc.) exists yet — that starts in
Phase 2.

## Prerequisites (Windows 11 Pro + Visual Studio 2026)

1. **Visual Studio 2026** with the "Desktop development with C++" workload
   (includes CMake, Ninja, and the MSVC toolset — you don't need to install
   these separately).
2. **vcpkg** — you have two options, and #1 is simpler if you don't already
   have a reason to prefer #2:

   **Option A — let VS manage it (recommended default).** VS 2026 ships with
   its own bundled vcpkg install (under its own Program Files folder) and
   uses it automatically for any CMake project with a `vcpkg.json` manifest,
   *regardless of `VCPKG_ROOT`*, unless that variable was already set in your
   environment before VS was launched. Nothing to install — just open the
   folder and build. This is what will happen by default even if you have a
   separate vcpkg elsewhere on disk.

   **Option B — force your own vcpkg install** (e.g. `D:\Projects\Tools\vcpkg`),
   useful if you want the exact same vcpkg version/state locally as in CI, or
   you're managing several projects against one shared cache:
   ```powershell
   git clone https://github.com/microsoft/vcpkg D:\Projects\Tools\vcpkg
   D:\Projects\Tools\vcpkg\bootstrap-vcpkg.bat
   setx VCPKG_ROOT D:\Projects\Tools\vcpkg
   ```
   Then **fully exit Visual Studio** (all windows, and check Task Manager for
   any lingering `devenv.exe`/`msbuild.exe` — closing the solution isn't
   enough) before reopening it, so the new environment variable actually
   reaches VS's process. If VS still uses its own bundled copy afterwards,
   check **Tools → Options → CMake** for a vcpkg-integration toggle to
   disable, since VS's own copy otherwise takes precedence in some setups.

## Debugging during development

Because of the Release-only Qt platform plugin (see "Known limitation"
below), **use `windows-x64-relwithdebinfo` (or `windows-x64-local`, which is
the same config with the hardcoded vcpkg path — see §4.8 of the roadmap) as
your daily driver in VS**, not `windows-x64-debug`, for anything that
touches `lancue-settings`:

- It links against Release-mode Qt (matching CRT/ABI), so `qwindows.dll`
  loads fine — no more "incompatible Qt library" crash.
- It still produces full PDBs, so breakpoints, stepping, watches, and
  variable inspection in VS all work normally.
- Optimizations are turned back off (`/Od`) for LanCue's own code only — Qt's
  prebuilt binaries stay untouched — so stepping through `core-lib`,
  `lancue-core`, and `lancue-settings` source feels just like a real Debug
  build; only Qt's own internals (which you're rarely stepping into anyway)
  stay optimized.

`lancue-core` has no Qt plugin dependency and works fine in plain Debug too,
but using `windows-x64-relwithdebinfo` for the whole solution avoids having
to juggle two different configurations for two executables in the same
solution.

`windows-x64-debug` is still available and useful for things unrelated to
Qt's plugin loading (e.g. debugging `core-lib` logic in isolation via the
test targets once Phase 1 adds them).

## Building

### From Visual Studio 2026

1. **File → Open → Folder…** and select the `lancue/` folder (the one
   containing this README and `CMakeLists.txt`).
2. VS reads `CMakePresets.json` automatically and offers `windows-x64-debug`,
   `windows-x64-local`, `windows-x64-release`, and `windows-x64-relwithdebinfo`
   in the configuration dropdown at the top toolbar. **Use `windows-x64-local`**
   if your vcpkg lives at `D:\Projects\Tools\vcpkg` — it's the same config as
   `windows-x64-relwithdebinfo` (see "Debugging during development" above;
   this matters — `windows-x64-debug` will crash `lancue-settings` on
   startup, see "Known limitation" below) but with `CMAKE_TOOLCHAIN_FILE`
   pointed straight at that install instead of relying on `VCPKG_ROOT`/VS's
   own bundled vcpkg (see §4.8 of the roadmap for why this preset exists
   separately from the others).
3. Pick a configuration, wait for CMake generation to finish (status bar),
   then **Build → Build All**.
4. First configure will take a while — vcpkg is compiling Qt from source.
   Subsequent configures reuse the cached build.
5. Run/debug `lancue-core` or `lancue-settings` from the target selector next
   to the Run button.

### From the command line

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
```

Both executables land under `build\windows-x64-debug\`. Run `lancue-core`
first (it has no console window once built with the default WIN32 subsystem
on Windows — check `%LOCALAPPDATA%\lancue-core\logs\lancue-core.log` for
startup confirmation), then run `lancue-settings`, which connects to it,
sends a test message, and logs the reply before exiting.

## Notes

- `vcpkg.json` uses manifest mode (no `vcpkg install` step needed — CMake
  triggers it automatically from `VCPKG_ROOT` on first configure).
- `vcpkg.json` pins `builtin-baseline` to a specific vcpkg commit for
  reproducible builds. This value is **locked** per the roadmap's §4.8 —
  don't bump it with `vcpkg x-update-baseline` unless the user explicitly
  asks for a baseline update.
- Warnings are treated as errors on every platform (see `cmake/*-toolchain.cmake`)
  per Phase 0's CI requirement — a warning in new code fails the build, not
  just CI.
- `lancue-settings` needs Qt's "platforms" plugin at runtime (it's a Widgets
  app); the build automatically copies it next to the .exe and generates a
  `qt.conf` on every build (see `cmake/qt-deploy-platform-plugin.cmake`).
  `lancue-core` doesn't need this — it's `QCoreApplication`-only and never
  loads a GUI platform plugin, which is why it ran fine without any extra
  step.
- **Known limitation:** this vcpkg `qtbase` build only ships a Release-mode
  `qwindows.dll`, and Qt refuses to load a Release plugin into a Debug
  executable. See "Debugging during development" above for the recommended
  workaround (`windows-x64-relwithdebinfo`) — it avoids this entirely while
  keeping full breakpoint debugging.
- **Local disk usage (`build/` + `.vs/`):** both are fully regenerable and
  safe to delete anytime (close Visual Studio first if deleting `.vs/`).
  The biggest space cost is `vcpkg_installed/` (Qt built from source, both
  Debug and Release, per triplet) — `base`'s `VCPKG_INSTALLED_DIR` already
  points every Windows preset at one shared `build/vcpkg_installed/`
  instead of each preset building its own copy, which is most of the
  savings available without switching triplets. What's left to prune
  manually:
  - Delete `build/<preset-name>/` for any preset you aren't actively using
    (e.g. if you only ever build `windows-x64-local`, the `-debug`,
    `-release`, and `-relwithdebinfo` build dirs can go — CMake regenerates
    them from `CMakePresets.json` on next configure).
  - `.vs/` is VS's IntelliSense/cache database, not build output — deleting
    it just costs one slower reindex on next open, nothing else.
  - Object files (`build/<preset>/**/*.obj`) accumulate across incremental
    builds; a full `build/<preset>/` delete + reconfigure clears them
    without needing to touch the shared `vcpkg_installed/`.
