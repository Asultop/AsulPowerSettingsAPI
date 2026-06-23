#pragma once

#include <string>
#include <vector>

namespace asul {

class CliApp {
public:
    CliApp();
    int run(int argc, char* argv[]);

private:
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
};

} // namespace asul
