#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace asul {

enum class OutputFormat { Text, Json, Csv };

class CliApp {
public:
    CliApp();
    int run(int argc, char* argv[]);

private:
    OutputFormat format_ = OutputFormat::Text;

    // Parse --format from argv, returns true if found
    bool parseFormat(int& argc, char* argv[]);

    void printUsage(const std::string &argv0) const;
    void printVersion(const std::string &argv0) const;

    int cmdList(int argc, char* argv[]);
    int cmdActive(int argc, char* argv[]);
    int cmdSetActive(int argc, char* argv[]);
    int cmdSubgroups(int argc, char* argv[]);
    int cmdSettings(int argc, char* argv[]);
    int cmdGet(int argc, char* argv[]);
    int cmdSet(int argc, char* argv[]);
    int cmdHidden(int argc, char* argv[]);
    int cmdScan(int argc, char* argv[]);
    int cmdCreate(int argc, char* argv[]);
    int cmdDuplicate(int argc, char* argv[]);
    int cmdDelete(int argc, char* argv[]);
    int cmdRename(int argc, char* argv[]);
    int cmdImport(int argc, char* argv[]);
    int cmdExport(int argc, char* argv[]);
    int cmdCompare(int argc, char* argv[]);
    int cmdESportsMode(int argc, char* argv[]);

    // JSON helpers
    static std::string jsonEscape(const std::string& s);
};

} // namespace asul
