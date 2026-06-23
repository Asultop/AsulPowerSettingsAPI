#include <cli/CliApp.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <string>
#include <vector>

static std::string wideToUtf8(const wchar_t* wstr) {
    if (!wstr) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(static_cast<size_t>(len) - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
    return result;
}

int wmain(int argc, wchar_t* argv[]) {
    // Convert UTF-16 argv to UTF-8
    std::vector<std::string> argsUtf8(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        argsUtf8[i] = wideToUtf8(argv[i]);
    }
    std::vector<char*> utf8Argv(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        utf8Argv[i] = &argsUtf8[i][0];
    }

    asul::CliApp app;
    return app.run(argc, utf8Argv.data());
}
