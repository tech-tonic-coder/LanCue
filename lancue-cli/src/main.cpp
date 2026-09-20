#include <cstdio>

#include <QFile>
#include <QString>
#include <QStringList>

#include "corelib/conversion/conversion_engine.h"
#include "corelib/conversion/layout_char_maps.h"

#ifdef _WIN32
#include <io.h>
// windows.h MUST come before shellapi.h: shellapi.h's own declarations
// depend on macros/types (EXTERN_C, DECLSPEC_IMPORT, HDROP, etc.) that
// only exist after windows.h's include chain has run. Including it first
// (an ordering bug introduced when CommandLineToArgvW support was added)
// caused a real MSVC build failure — cascading C2146/C2086/C3646/C3861
// errors inside shellapi.h itself, culminating in "CommandLineToArgvW:
// identifier not found" even though the header was nominally included —
// found on Mahdi's own machine with the windows-x64-relwithdebinfo
// preset (2026-08-27). This #include order is load-bearing; don't
// reorder it back.
#include <windows.h>
#include <shellapi.h>
#if defined(_MSC_VER)
// CommandLineToArgvW lives in Shell32.lib, which isn't guaranteed to be
// in every MSVC project's default link set the way Kernel32/User32 are —
// pragma-linking it here (rather than only via CMake) keeps this file
// buildable regardless of how the target's link libraries end up
// configured, matching how real-world Windows tools commonly guarantee
// this exact API resolves.
#pragma comment(lib, "shell32.lib")
#endif
#endif

namespace {

// Reads all of stdin as text. On Windows, an *interactive* console is
// read via ReadConsoleW (UTF-16 directly from the console buffer) rather
// than through the CRT's narrow-char stdin, which would otherwise
// translate through the console's ANSI/OEM codepage and corrupt any
// Persian character in typed input — the same category of Windows
// console-encoding gotcha documented for writeStdout() below, and for
// the same reason: this tool's entire purpose is correctly round-
// tripping non-ASCII text. A *redirected* stdin (the far more common
// case for a pipe tool — see this phase's own roadmap text) is not a
// console at all, so no codepage translation happens regardless of how
// it's read; raw bytes decoded as UTF-8 is both correct and simplest
// there, and is the only path needed on macOS/Linux, where the terminal
// is UTF-8 by default.
QString readAllStdin() {
#ifdef _WIN32
    if (_isatty(_fileno(stdin))) {
        const HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
        QString result;
        wchar_t buffer[4096];
        DWORD charsRead = 0;
        while (ReadConsoleW(handle, buffer, 4096, &charsRead, nullptr) && charsRead > 0) {
            result += QString::fromWCharArray(buffer, static_cast<int>(charsRead));
        }
        return result;
    }
#endif
    QFile in;
    if (!in.open(stdin, QIODevice::ReadOnly)) {
        // MSVC's /W4 /WX (windows-toolchain.cmake) treats Qt6's
        // [[nodiscard]] on QIODevice::open() as a hard error if its
        // result is ignored — and ignoring it here would be a real bug
        // anyway, not just a warning to silence: an unopened QFile's
        // readAll() would silently return empty, indistinguishable from
        // genuinely empty stdin. Fail loudly instead.
        std::fprintf(stderr, "error: could not open stdin for reading\n");
        return QString();
    }
    return QString::fromUtf8(in.readAll());
}

// Mirror of readAllStdin()'s reasoning, for output: an interactive
// Windows console gets WriteConsoleW (writes UTF-16 straight to the
// console buffer, bypassing codepage translation entirely); a redirected
// stdout gets raw UTF-8 bytes, which is both correct for a pipe/file and
// the only path needed on macOS/Linux.
//
// This makes the *bytes* correct either way. Whether Persian glyphs are
// actually visible in a given interactive console session also depends
// on that console's selected font supporting the Persian Unicode range
// (Windows' legacy Raster Fonts option does not) — a terminal
// configuration matter outside this program's control, not a bug in
// this function; worth knowing before assuming a blank/boxed glyph means
// broken conversion.
void writeStdout(const QString& text) {
#ifdef _WIN32
    if (_isatty(_fileno(stdout))) {
        const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD charsWritten = 0;
        WriteConsoleW(handle, text.utf16(), static_cast<DWORD>(text.length()), &charsWritten, nullptr);
        return;
    }
#endif
    QFile out;
    if (!out.open(stdout, QIODevice::WriteOnly)) {
        // Same reasoning as readAllStdin()'s open() check above.
        std::fprintf(stderr, "error: could not open stdout for writing\n");
        return;
    }
    out.write(text.toUtf8());
}

void printUsage(std::FILE* stream) {
    std::fprintf(stream,
                  "usage: lancue-cli convert --to <layout> [--from <layout>] [text]\n"
                  "       lancue-cli detect [text]\n"
                  "       lancue-cli list-layouts\n"
                  "       lancue-cli help\n"
                  "\n"
                  "  --to <layout>     target layout id, e.g. fa-ir (always required)\n"
                  "  --from <layout>   source layout id, e.g. en-us (optional — if omitted,\n"
                  "                    the source is auto-detected from the text itself, the\n"
                  "                    same way LanCue's own per-language hotkeys do)\n"
                  "  [text]            text to convert/detect; omit to read from stdin instead\n"
                  "\n"
                  "run 'lancue-cli list-layouts' for the full set of supported layout ids.\n"
                  "run 'lancue-cli detect [text]' to see what layout auto-detection would pick\n"
                  "for a given piece of text, without converting anything.\n"
                  "\n"
                  "examples:\n"
                  "  lancue-cli convert --from en-us --to fa-ir \"SLpi\"\n"
                  "  lancue-cli convert --to fa-ir \"SLpi\"                (auto-detects en-us)\n"
                  "  echo SLpi | lancue-cli convert --to fa-ir\n"
                  "  lancue-cli convert --from fa-ir --to ar-sa \"...\"    (pivots through en-us)\n"
                  "  lancue-cli detect \"\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85\"                                -> fa-ir\n");
}

// main()'s narrow `argv` is lossy on Windows for any character outside
// the process's current ANSI codepage: the CRT startup code converts the
// OS's actual wide (UTF-16) command line down to narrow using that
// codepage before main() ever sees it, and any character it can't
// represent silently becomes '?'. This is exactly what corrupted Persian/
// Arabic/Hebrew/Cyrillic text passed directly as a command-line argument
// (as opposed to piped through stdin, which readAllStdin() above already
// handles correctly via ReadConsoleW) — a well-documented, long-standing
// Windows gotcha, not specific to this project or this text. The
// standard fix, used by essentially every serious Windows CLI tool that
// needs correct Unicode arguments, is to bypass narrow argv entirely:
// re-fetch the original command line as UTF-16 via GetCommandLineW() and
// re-parse it with CommandLineToArgvW(), converting each wide argument
// straight to QString. Falls back to the (lossy, but non-fatal) narrow
// argv only if CommandLineToArgvW itself fails outright, which would be
// unusual.
QStringList parseCommandLineArgs(int argc, char* argv[]) {
#ifdef _WIN32
    int wideArgc = 0;
    LPWSTR* wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
    if (wideArgv) {
        QStringList args;
        for (int i = 1; i < wideArgc; ++i) {
            args << QString::fromWCharArray(wideArgv[i]);
        }
        LocalFree(wideArgv);
        return args;
    }
    std::fprintf(stderr, "warning: CommandLineToArgvW failed; falling back to possibly-lossy arguments\n");
#endif
    QStringList list;
    for (int i = 1; i < argc; ++i) {
        list << QString::fromLocal8Bit(argv[i]);
    }
    return list;
}

} // namespace

int main(int argc, char* argv[]) {
    const QStringList args = parseCommandLineArgs(argc, argv);

    if (args.isEmpty()) {
        printUsage(stderr);
        return 2;
    }

    const QString command = args.first();

    if (command == QStringLiteral("help") || command == QStringLiteral("--help") || command == QStringLiteral("-h")) {
        // Explicitly requested help is not an error condition — goes to
        // stdout with exit code 0, unlike the "you did something wrong"
        // usage printouts below (stderr, exit code 2), so scripting
        // `lancue-cli --help` doesn't look like a failure.
        printUsage(stdout);
        return 0;
    }

    if (command == QStringLiteral("list-layouts")) {
        for (const QString& id : lancue::conversion::supportedLayoutIds()) {
            writeStdout(id + QStringLiteral("\n"));
        }
        return 0;
    }

    if (command == QStringLiteral("detect")) {
        // Exposes conversion::detectLayout() directly — the same
        // function `convert`'s own --from auto-detection (below) and
        // ConversionFeature's dedicated per-language hotkeys use — so a
        // user or script can see exactly what auto-detection would pick
        // for a piece of text without actually converting it, useful for
        // understanding/debugging an unexpected --from-less conversion
        // result.
        QStringList textParts;
        for (int i = 1; i < args.size(); ++i) {
            textParts << args.at(i);
        }
        const QString inputText = textParts.isEmpty() ? readAllStdin() : textParts.join(QStringLiteral(" "));
        writeStdout(lancue::conversion::detectLayout(inputText) + QStringLiteral("\n"));
        return 0;
    }

    if (command != QStringLiteral("convert")) {
        printUsage(stderr);
        return 2;
    }

    QString fromLayout;
    QString toLayout;
    QStringList textParts;

    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args.at(i);
        if (arg == QStringLiteral("--from")) {
            if (i + 1 >= args.size()) {
                std::fprintf(stderr, "error: --from requires a value\n");
                return 2;
            }
            fromLayout = args.at(++i);
        } else if (arg == QStringLiteral("--to")) {
            if (i + 1 >= args.size()) {
                std::fprintf(stderr, "error: --to requires a value\n");
                return 2;
            }
            toLayout = args.at(++i);
        } else {
            // Any non-flag argument is part of the text to convert.
            // Joined with a single space if more than one is given, so
            // unquoted multi-word text on a shell that doesn't require
            // quoting still works reasonably — the documented example
            // usage quotes its text, but nothing here requires that.
            textParts << arg;
        }
    }

    if (toLayout.isEmpty()) {
        std::fprintf(stderr, "error: --to is required\n");
        printUsage(stderr);
        return 2;
    }

    const QString inputText = textParts.isEmpty() ? readAllStdin() : textParts.join(QStringLiteral(" "));

    if (fromLayout.isEmpty()) {
        // --from omitted: auto-detect the source from the text itself,
        // the same conversion::detectLayout() call ConversionFeature's
        // dedicated per-language hotkeys use for exactly this reason
        // (§5 Phase 6) — reusing that already-tested engine function
        // rather than duplicating its logic here. Noted on stderr, not
        // stdout, so it doesn't corrupt a script's captured converted
        // output; run 'lancue-cli detect' first for a non-interleaved
        // look at what this would pick.
        fromLayout = lancue::conversion::detectLayout(inputText);
        std::fprintf(stderr, "note: --from omitted; auto-detected source layout '%s'\n", qPrintable(fromLayout));
    }

    const lancue::conversion::ConversionResult result = lancue::conversion::convert(inputText, fromLayout, toLayout);
    if (!result.ok) {
        std::fprintf(stderr, "error: no character map from '%s' to '%s'\n", qPrintable(fromLayout),
                      qPrintable(toLayout));
        std::fprintf(stderr, "run 'lancue-cli list-layouts' to see supported layout ids\n");
        return 1;
    }

    writeStdout(result.text);
    return 0;
}
