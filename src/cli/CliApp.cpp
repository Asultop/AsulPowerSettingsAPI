#include <cli/CliApp.h>
#include <AsulPowerSettingsApi/PowerSettingsManager.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <objbase.h>

#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <string_view>
#include <map>
#include <sstream>

namespace asul {

namespace {

// Parse GUID from UTF-8 string, converting to UTF-16 for CLSIDFromString
bool parseGuid(const std::string& utf8Str, GUID& outGuid) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return false;
    std::wstring wstr(static_cast<size_t>(wlen) - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wstr[0], wlen);
    return CLSIDFromString(wstr.c_str(), &outGuid) == S_OK;
}

} // anonymous namespace

CliApp::CliApp() = default;

// ---------------------------------------------------------------------------
// Format parsing
// ---------------------------------------------------------------------------
bool CliApp::parseFormat(int& argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--format") == 0 && i + 1 < argc) {
            std::string val = argv[i + 1];
            if (val == "json") format_ = OutputFormat::Json;
            else if (val == "csv") format_ = OutputFormat::Csv;
            else format_ = OutputFormat::Text;
            // Remove --format and its value from argv
            for (int j = i; j + 2 < argc; ++j) argv[j] = argv[j + 2];
            argc -= 2;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// JSON helper
// ---------------------------------------------------------------------------
std::string CliApp::jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Usage / Version
// ---------------------------------------------------------------------------
void CliApp::printVersion(const std::string &argv0) const {
    std::cout << argv0 << " v1.0.0" << std::endl;
    std::cout << "Windows 电源设置管理器 (SDK v1.0.0)" << std::endl;
}

void CliApp::printUsage(const std::string &argv0) const {
    printVersion(argv0);
    std::cout << R"(
用法: power-settings <命令> [选项]

命令:
  list                          列出所有电源方案
  active                        显示当前活动电源方案 GUID
  set-active <GUID>             设置活动电源方案
  subgroups <方案-GUID>         列出方案下的子组
  settings <方案-GUID> <子组-GUID>  列出子组下的设置项
  get <方案-GUID> <子组-GUID> <设置-GUID>  读取当前 AC/DC 值
  set <方案-GUID> <子组-GUID> <设置-GUID> --ac <值> --dc <值>  写入值
  hidden                        列出被隐藏的电源设置（来自注册表）
  scan                          完整扫描：枚举所有方案/子组/设置
  create <源GUID> <名称>        基于现有方案创建新的电源方案
  duplicate <GUID>              复制电源方案
  delete <GUID>                 删除电源方案
  rename <GUID> <新名称>        重命名电源方案
  import <文件路径>             从 .pow 文件导入电源方案
  export <GUID> <文件路径>      导出电源方案为 .pow 文件
  compare <GUID1> <GUID2>       对比两个电源方案的设置差异
  addESportsMode                添加电竞模式电源方案
  help                          显示此帮助信息
  version                       显示版本

选项:
  --format <text|json|csv>      输出格式（默认 text）
)" << std::endl;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int CliApp::run(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    if (cmd == "version" || cmd == "--version") {
        printVersion(argv[0]);
        return 0;
    }

    // Parse --format globally (skip argv[0] and argv[1]=command)
    parseFormat(argc, argv);

    if (cmd == "list")        return cmdList(argc, argv);
    if (cmd == "active")      return cmdActive(argc, argv);
    if (cmd == "set-active")  return cmdSetActive(argc, argv);
    if (cmd == "subgroups")   return cmdSubgroups(argc, argv);
    if (cmd == "settings")    return cmdSettings(argc, argv);
    if (cmd == "get")         return cmdGet(argc, argv);
    if (cmd == "set")         return cmdSet(argc, argv);
    if (cmd == "hidden")      return cmdHidden(argc, argv);
    if (cmd == "scan")        return cmdScan(argc, argv);
    if (cmd == "create")      return cmdCreate(argc, argv);
    if (cmd == "duplicate")   return cmdDuplicate(argc, argv);
    if (cmd == "delete")      return cmdDelete(argc, argv);
    if (cmd == "rename")      return cmdRename(argc, argv);
    if (cmd == "import")      return cmdImport(argc, argv);
    if (cmd == "export")      return cmdExport(argc, argv);
    if (cmd == "compare")     return cmdCompare(argc, argv);
    if (cmd == "addESportsMode") return cmdESportsMode(argc, argv);

    std::cerr << "未知命令: " << cmd << std::endl;
    printUsage(argv[0]);
    return 1;
}

// ---------------------------------------------------------------------------
// list
// ---------------------------------------------------------------------------
int CliApp::cmdList(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    if (ec) {
        std::cerr << "枚举电源方案失败: " << ec.message() << std::endl;
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "[\n";
        for (size_t i = 0; i < schemes.size(); ++i) {
            const auto& s = schemes[i];
            std::cout << "  {\"guid\": \"" << jsonEscape(s.guid.toString())
                      << "\", \"name\": \"" << jsonEscape(s.name) << "\"}";
            if (i + 1 < schemes.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "guid,name" << std::endl;
        for (const auto& s : schemes) {
            std::cout << s.guid.toString() << ",\"" << jsonEscape(s.name) << "\"" << std::endl;
        }
    } else {
        std::cout << "电源方案 (" << schemes.size() << " 个):" << std::endl;
        for (const auto& s : schemes) {
            std::cout << "  " << s.guid.toString() << "  " << s.name << std::endl;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// active
// ---------------------------------------------------------------------------
int CliApp::cmdActive(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    auto guid = mgr.getActiveScheme(ec);
    if (ec) {
        std::cerr << "错误: " << ec.message() << std::endl;
        return 1;
    }
    std::string name = mgr.readFriendlyName(guid, ec);

    if (format_ == OutputFormat::Json) {
        std::cout << "{\"guid\": \"" << jsonEscape(guid.toString())
                  << "\", \"name\": \"" << jsonEscape(name) << "\"}" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "guid,name" << std::endl;
        std::cout << guid.toString() << ",\"" << jsonEscape(name) << "\"" << std::endl;
    } else {
        std::cout << "当前活动方案: " << guid.toString();
        if (!name.empty()) std::cout << "  (" << name << ")";
        std::cout << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// set-active
// ---------------------------------------------------------------------------
int CliApp::cmdSetActive(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings set-active <GUID>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    GUID g;
    if (!parseGuid(guidStr, g)) {
        std::cerr << "无效的 GUID: " << guidStr << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto schemeGuid = fromWindowsGuid(g);
    if (!mgr.setActiveScheme(schemeGuid, ec)) {
        std::cerr << "错误: " << ec.message() << std::endl;
        return 1;
    }
    if (format_ == OutputFormat::Json) {
        std::cout << "{\"success\": true, \"guid\": \"" << jsonEscape(schemeGuid.toString()) << "\"}" << std::endl;
    } else {
        std::cout << "已切换活动方案为 " << schemeGuid.toString() << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// subgroups
// ---------------------------------------------------------------------------
int CliApp::cmdSubgroups(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings subgroups <方案-GUID>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    GUID g;
    if (!parseGuid(guidStr, g)) {
        std::cerr << "无效的 GUID: " << guidStr << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto schemeGuid = fromWindowsGuid(g);
    auto subgroups = mgr.enumerateSubgroups(schemeGuid, ec);
    if (ec) {
        std::cerr << "错误: " << ec.message() << std::endl;
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "[\n";
        for (size_t i = 0; i < subgroups.size(); ++i) {
            const auto& sg = subgroups[i];
            std::cout << "  {\"guid\": \"" << jsonEscape(sg.guid.toString())
                      << "\", \"name\": \"" << jsonEscape(sg.name)
                      << "\", \"hidden\": " << (sg.isHidden() ? "true" : "false") << "}";
            if (i + 1 < subgroups.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "guid,name,hidden" << std::endl;
        for (const auto& sg : subgroups) {
            std::cout << sg.guid.toString() << ",\""
                      << jsonEscape(sg.name) << "\","
                      << (sg.isHidden() ? "true" : "false") << std::endl;
        }
    } else {
        std::cout << "子组 (" << subgroups.size() << " 个):" << std::endl;
        for (const auto& sg : subgroups) {
            std::string hidden = sg.isHidden() ? " [隐藏]" : "";
            std::cout << "  " << sg.guid.toString() << "  " << sg.name << hidden << std::endl;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// settings
// ---------------------------------------------------------------------------
int CliApp::cmdSettings(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: power-settings settings <方案-GUID> <子组-GUID>" << std::endl;
        return 1;
    }
    std::string schemeStr = argv[2];
    std::string subStr = argv[3];

    GUID schemeG, subG;
    if (!parseGuid(schemeStr, schemeG) || !parseGuid(subStr, subG)) {
        std::cerr << "无效的 GUID" << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto settings = mgr.enumerateSettings(fromWindowsGuid(schemeG),
                                          fromWindowsGuid(subG), ec);
    if (ec) {
        std::cerr << "错误: " << ec.message() << std::endl;
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "[\n";
        for (size_t i = 0; i < settings.size(); ++i) {
            const auto& s = settings[i];
            std::cout << "  {\"guid\": \"" << jsonEscape(s.guid.toString())
                      << "\", \"name\": \"" << jsonEscape(s.name)
                      << "\", \"hidden\": " << (s.isHidden() ? "true" : "false")
                      << ", \"ac_default\": " << s.acDefault
                      << ", \"dc_default\": " << s.dcDefault << "}";
            if (i + 1 < settings.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "guid,name,hidden,ac_default,dc_default" << std::endl;
        for (const auto& s : settings) {
            std::cout << s.guid.toString() << ",\""
                      << jsonEscape(s.name) << "\","
                      << (s.isHidden() ? "true" : "false") << ","
                      << s.acDefault << "," << s.dcDefault << std::endl;
        }
    } else {
        std::cout << "设置项 (" << settings.size() << " 个):" << std::endl;
        for (const auto& s : settings) {
            std::string hidden = s.isHidden() ? " [隐藏]" : "";
            std::cout << "  " << s.guid.toString() << "  " << s.name << hidden << std::endl;
            std::cout << "    AC 默认=" << s.acDefault
                      << "  DC 默认=" << s.dcDefault << std::endl;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// get
// ---------------------------------------------------------------------------
int CliApp::cmdGet(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "用法: power-settings get <方案-GUID> <子组-GUID> <设置-GUID>"
                  << std::endl;
        return 1;
    }
    std::string schemeStr = argv[2];
    std::string subStr = argv[3];
    std::string setStr = argv[4];

    GUID schemeG, subG, setG;
    if (!parseGuid(schemeStr, schemeG) || !parseGuid(subStr, subG) || !parseGuid(setStr, setG)) {
        std::cerr << "无效的 GUID" << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    uint32_t acVal = 0, dcVal = 0;
    bool acOk = mgr.readACValueIndex(fromWindowsGuid(schemeG),
                                      fromWindowsGuid(subG),
                                      fromWindowsGuid(setG), acVal, ec);
    bool dcOk = mgr.readDCValueIndex(fromWindowsGuid(schemeG),
                                      fromWindowsGuid(subG),
                                      fromWindowsGuid(setG), dcVal, ec);

    if (format_ == OutputFormat::Json) {
        std::cout << "{\"setting\": \"" << jsonEscape(setStr)
                  << "\", \"ac\": " << (acOk ? std::to_string(acVal) : "null")
                  << ", \"dc\": " << (dcOk ? std::to_string(dcVal) : "null")
                  << "}" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "setting,ac,dc" << std::endl;
        std::cout << setStr << ","
                  << (acOk ? std::to_string(acVal) : "") << ","
                  << (dcOk ? std::to_string(dcVal) : "") << std::endl;
    } else {
        std::cout << "设置项: " << setStr << std::endl;
        std::cout << "  AC 值: " << (acOk ? std::to_string(acVal) : "不可用") << std::endl;
        std::cout << "  DC 值: " << (dcOk ? std::to_string(dcVal) : "不可用") << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// set
// ---------------------------------------------------------------------------
int CliApp::cmdSet(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "用法: power-settings set <方案-GUID> <子组-GUID> <设置-GUID>"
                  << " --ac <值> --dc <值>" << std::endl;
        return 1;
    }

    std::string schemeStr = argv[2];
    std::string subStr = argv[3];
    std::string setStr = argv[4];

    uint32_t acVal = UINT32_MAX, dcVal = UINT32_MAX;
    for (int i = 5; i + 1 < argc; i += 2) {
        if (std::strcmp(argv[i], "--ac") == 0) {
            acVal = static_cast<uint32_t>(std::strtoul(argv[i + 1], nullptr, 10));
        } else if (std::strcmp(argv[i], "--dc") == 0) {
            dcVal = static_cast<uint32_t>(std::strtoul(argv[i + 1], nullptr, 10));
        }
    }

    GUID schemeG, subG, setG;
    if (!parseGuid(schemeStr, schemeG) || !parseGuid(subStr, subG) || !parseGuid(setStr, setG)) {
        std::cerr << "无效的 GUID" << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto sg = fromWindowsGuid(schemeG);
    auto sub = fromWindowsGuid(subG);
    auto set = fromWindowsGuid(setG);

    int rc = 0;
    bool acOk = true, dcOk = true;
    if (acVal != UINT32_MAX) {
        acOk = mgr.writeACValueIndex(sg, sub, set, acVal, ec);
        if (!acOk) {
            std::cerr << "写入 AC 值失败: " << ec.message() << std::endl;
            rc = 1;
        }
    }
    if (dcVal != UINT32_MAX) {
        dcOk = mgr.writeDCValueIndex(sg, sub, set, dcVal, ec);
        if (!dcOk) {
            std::cerr << "写入 DC 值失败: " << ec.message() << std::endl;
            rc = 1;
        }
    }
    if (acVal == UINT32_MAX && dcVal == UINT32_MAX) {
        std::cerr << "未指定要设置的值，请使用 --ac <值> 和/或 --dc <值>" << std::endl;
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "{\"success\": " << (rc == 0 ? "true" : "false");
        if (acVal != UINT32_MAX) std::cout << ", \"ac\": " << acVal;
        if (dcVal != UINT32_MAX) std::cout << ", \"dc\": " << dcVal;
        std::cout << "}" << std::endl;
    } else if (rc == 0) {
        if (acVal != UINT32_MAX) std::cout << "AC 值已设置为 " << acVal << std::endl;
        if (dcVal != UINT32_MAX) std::cout << "DC 值已设置为 " << dcVal << std::endl;
    }
    return rc;
}

// ---------------------------------------------------------------------------
// hidden
// ---------------------------------------------------------------------------
int CliApp::cmdHidden(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    auto hidden = mgr.enumerateHiddenSettings(ec);
    if (ec) {
        std::cerr << "读取注册表失败: " << ec.message() << std::endl;
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "[\n";
        for (size_t i = 0; i < hidden.size(); ++i) {
            const auto& s = hidden[i];
            std::cout << "  {\"guid\": \"" << jsonEscape(s.guid.toString())
                      << "\", \"name\": \"" << jsonEscape(s.name) << "\"}";
            if (i + 1 < hidden.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "guid,name" << std::endl;
        for (const auto& s : hidden) {
            std::cout << s.guid.toString() << ",\"" << jsonEscape(s.name) << "\"" << std::endl;
        }
    } else {
        std::cout << "被隐藏的电源设置 (" << hidden.size() << " 个):" << std::endl;
        for (const auto& s : hidden) {
            std::cout << "  " << s.guid.toString() << "  " << s.name << std::endl;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// scan
// ---------------------------------------------------------------------------
int CliApp::cmdScan(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    if (format_ == OutputFormat::Text)
        std::cout << "正在执行完整电源设置扫描..." << std::endl;

    auto schemes = mgr.fullScan(ec);
    if (ec) {
        std::cerr << "扫描出错: " << ec.message() << std::endl;
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "[\n";
        for (size_t si = 0; si < schemes.size(); ++si) {
            const auto& scheme = schemes[si];
            std::cout << "  {\"guid\": \"" << jsonEscape(scheme.guid.toString())
                      << "\", \"name\": \"" << jsonEscape(scheme.name)
                      << "\", \"subgroups\": [\n";
            for (size_t gi = 0; gi < scheme.subgroups.size(); ++gi) {
                const auto& sg = scheme.subgroups[gi];
                std::cout << "    {\"guid\": \"" << jsonEscape(sg.guid.toString())
                          << "\", \"name\": \"" << jsonEscape(sg.name)
                          << "\", \"hidden\": " << (sg.isHidden() ? "true" : "false")
                          << ", \"settings\": [\n";
                for (size_t ti = 0; ti < sg.settings.size(); ++ti) {
                    const auto& st = sg.settings[ti];
                    std::cout << "      {\"guid\": \"" << jsonEscape(st.guid.toString())
                              << "\", \"name\": \"" << jsonEscape(st.name)
                              << "\", \"hidden\": " << (st.isHidden() ? "true" : "false")
                              << ", \"ac\": " << st.acValueOverride
                              << ", \"dc\": " << st.dcValueOverride
                              << ", \"ac_default\": " << st.acDefault
                              << ", \"dc_default\": " << st.dcDefault << "}";
                    if (ti + 1 < sg.settings.size()) std::cout << ",";
                    std::cout << "\n";
                }
                std::cout << "    ]}";
                if (gi + 1 < scheme.subgroups.size()) std::cout << ",";
                std::cout << "\n";
            }
            std::cout << "  ]}";
            if (si + 1 < schemes.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "scheme_guid,scheme_name,subgroup_guid,subgroup_name,setting_guid,setting_name,ac,dc,ac_default,dc_default" << std::endl;
        for (const auto& scheme : schemes) {
            for (const auto& sg : scheme.subgroups) {
                for (const auto& st : sg.settings) {
                    std::cout << scheme.guid.toString() << ",\""
                              << jsonEscape(scheme.name) << "\","
                              << sg.guid.toString() << ",\""
                              << jsonEscape(sg.name) << "\","
                              << st.guid.toString() << ",\""
                              << jsonEscape(st.name) << "\","
                              << st.acValueOverride << ","
                              << st.dcValueOverride << ","
                              << st.acDefault << ","
                              << st.dcDefault << std::endl;
                }
            }
        }
    } else {
        for (const auto& scheme : schemes) {
            std::cout << "\n=== 电源方案: " << scheme.name
                      << " " << scheme.guid.toString() << " ===" << std::endl;
            for (const auto& subgroup : scheme.subgroups) {
                std::string sgHidden = subgroup.isHidden() ? " [隐藏]" : "";
                std::cout << "  [子组] " << subgroup.name
                          << " " << subgroup.guid.toString() << sgHidden << std::endl;
                for (const auto& setting : subgroup.settings) {
                    std::string sHidden = setting.isHidden() ? " [隐藏]" : "";
                    std::cout << "    [设置] " << setting.name
                              << " " << setting.guid.toString() << sHidden << std::endl;
                    std::cout << "      AC=" << setting.acValueOverride
                              << "  DC=" << setting.dcValueOverride
                              << "  (默认: AC=" << setting.acDefault
                              << " DC=" << setting.dcDefault << ")" << std::endl;
                }
            }
        }
        std::cout << "\n扫描完成，共发现 "
                  << schemes.size() << " 个电源方案。" << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// create
// ---------------------------------------------------------------------------
int CliApp::cmdCreate(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: power-settings create <源方案-GUID> <新方案名称>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    std::string name = argv[3];

    GUID g;
    if (!parseGuid(guidStr, g)) {
        std::cerr << "无效的 GUID: " << guidStr << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto newGuid = mgr.createScheme(fromWindowsGuid(g), name, ec);
    if (ec) {
        std::cerr << "创建电源方案失败: " << ec.message() << std::endl;
        return 1;
    }
    if (format_ == OutputFormat::Json) {
        std::cout << "{\"guid\": \"" << jsonEscape(newGuid.toString())
                  << "\", \"name\": \"" << jsonEscape(name) << "\"}" << std::endl;
    } else {
        std::cout << "已创建电源方案 \"" << name << "\"" << std::endl;
        std::cout << "  GUID: " << newGuid.toString() << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// duplicate
// ---------------------------------------------------------------------------
int CliApp::cmdDuplicate(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings duplicate <方案-GUID>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];

    GUID g;
    if (!parseGuid(guidStr, g)) {
        std::cerr << "无效的 GUID: " << guidStr << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto newGuid = mgr.duplicateScheme(fromWindowsGuid(g), ec);
    if (ec) {
        std::cerr << "复制电源方案失败: " << ec.message() << std::endl;
        return 1;
    }
    std::string name = mgr.readFriendlyName(newGuid, ec);

    if (format_ == OutputFormat::Json) {
        std::cout << "{\"guid\": \"" << jsonEscape(newGuid.toString())
                  << "\", \"name\": \"" << jsonEscape(name) << "\"}" << std::endl;
    } else {
        std::cout << "已复制电源方案" << std::endl;
        std::cout << "  GUID: " << newGuid.toString();
        if (!name.empty()) std::cout << "  (" << name << ")";
        std::cout << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// delete
// ---------------------------------------------------------------------------
int CliApp::cmdDelete(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings delete <方案-GUID>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];

    GUID g;
    if (!parseGuid(guidStr, g)) {
        std::cerr << "无效的 GUID: " << guidStr << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    if (!mgr.deleteScheme(fromWindowsGuid(g), ec)) {
        std::cerr << "删除电源方案失败: " << ec.message() << std::endl;
        return 1;
    }
    if (format_ == OutputFormat::Json) {
        std::cout << "{\"success\": true, \"guid\": \"" << jsonEscape(guidStr) << "\"}" << std::endl;
    } else {
        std::cout << "已删除电源方案 " << guidStr << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// rename
// ---------------------------------------------------------------------------
int CliApp::cmdRename(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: power-settings rename <方案-GUID> <新名称>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    std::string newName = argv[3];

    GUID g;
    if (!parseGuid(guidStr, g)) {
        std::cerr << "无效的 GUID: " << guidStr << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    if (!mgr.renameScheme(fromWindowsGuid(g), newName, ec)) {
        std::cerr << "重命名电源方案失败: " << ec.message() << std::endl;
        return 1;
    }
    if (format_ == OutputFormat::Json) {
        std::cout << "{\"success\": true, \"guid\": \"" << jsonEscape(guidStr)
                  << "\", \"name\": \"" << jsonEscape(newName) << "\"}" << std::endl;
    } else {
        std::cout << "已将电源方案重命名为 \"" << newName << "\"" << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------
int CliApp::cmdImport(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings import <.pow文件路径>" << std::endl;
        return 1;
    }
    std::string filePath = argv[2];

    PowerSettingsManager mgr;
    std::error_code ec;
    auto guid = mgr.importScheme(filePath, ec);
    if (ec) {
        std::cerr << "导入电源方案失败: " << ec.message() << std::endl;
        return 1;
    }
    std::string name = mgr.readFriendlyName(guid, ec);

    if (format_ == OutputFormat::Json) {
        std::cout << "{\"guid\": \"" << jsonEscape(guid.toString())
                  << "\", \"name\": \"" << jsonEscape(name) << "\"}" << std::endl;
    } else {
        std::cout << "已导入电源方案" << std::endl;
        std::cout << "  GUID: " << guid.toString();
        if (!name.empty()) std::cout << "  (" << name << ")";
        std::cout << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// export
// ---------------------------------------------------------------------------
int CliApp::cmdExport(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: power-settings export <方案-GUID> <.pow文件路径>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    std::string filePath = argv[3];

    // Build powercfg /export command: powercfg /export <path> <guid-without-braces>
    std::string cleanGuid = guidStr;
    cleanGuid.erase(std::remove(cleanGuid.begin(), cleanGuid.end(), '{'), cleanGuid.end());
    cleanGuid.erase(std::remove(cleanGuid.begin(), cleanGuid.end(), '}'), cleanGuid.end());

    std::string cmd = "powercfg /export \"" + filePath + "\" " + cleanGuid;
    int rc = system(cmd.c_str());

    if (rc != 0) {
        std::cerr << "导出电源方案失败 (错误码: " << rc << ")" << std::endl;
        if (format_ == OutputFormat::Json) {
            std::cout << "{\"success\": false, \"error\": \"export failed\"}" << std::endl;
        }
        return 1;
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "{\"success\": true, \"guid\": \"" << jsonEscape(guidStr)
                  << "\", \"path\": \"" << jsonEscape(filePath) << "\"}" << std::endl;
    } else {
        std::cout << "已导出电源方案 " << guidStr << " 到 " << filePath << std::endl;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// compare
// ---------------------------------------------------------------------------
int CliApp::cmdCompare(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: power-settings compare <方案1-GUID> <方案2-GUID>" << std::endl;
        return 1;
    }

    GUID g1, g2;
    if (!parseGuid(argv[2], g1) || !parseGuid(argv[3], g2)) {
        std::cerr << "无效的 GUID" << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto sg1 = fromWindowsGuid(g1);
    auto sg2 = fromWindowsGuid(g2);

    // Full scan both schemes
    auto scan1 = mgr.fullScan(ec);
    if (ec) { std::cerr << "扫描失败: " << ec.message() << std::endl; return 1; }
    auto scan2 = mgr.fullScan(ec);
    if (ec) { std::cerr << "扫描失败: " << ec.message() << std::endl; return 1; }

    // Find the two schemes
    const PowerScheme* scheme1 = nullptr;
    const PowerScheme* scheme2 = nullptr;
    for (const auto& s : scan1) { if (s.guid == sg1) scheme1 = &s; }
    for (const auto& s : scan2) { if (s.guid == sg2) scheme2 = &s; }

    if (!scheme1) {
        std::cerr << "未找到方案: " << argv[2] << std::endl;
        return 1;
    }
    if (!scheme2) {
        std::cerr << "未找到方案: " << argv[3] << std::endl;
        return 1;
    }

    // Build maps: setting GUID -> {subgroup name, setting name, ac, dc}
    struct SettingInfo {
        std::string subgroupName;
        std::string settingName;
        uint32_t ac;
        uint32_t dc;
    };
    std::map<std::string, SettingInfo> map1, map2;

    for (const auto& sg : scheme1->subgroups) {
        for (const auto& st : sg.settings) {
            map1[st.guid.toString()] = {sg.name, st.name, st.acValueOverride, st.dcValueOverride};
        }
    }
    for (const auto& sg : scheme2->subgroups) {
        for (const auto& st : sg.settings) {
            map2[st.guid.toString()] = {sg.name, st.name, st.acValueOverride, st.dcValueOverride};
        }
    }

    // Find differences
    struct Diff {
        std::string settingGuid;
        std::string settingName;
        std::string subgroupName;
        uint32_t ac1, dc1, ac2, dc2;
    };
    std::vector<Diff> diffs;

    for (const auto& kv : map1) {
        const auto& guid = kv.first;
        const auto& info1 = kv.second;
        auto it = map2.find(guid);
        if (it != map2.end()) {
            const auto& info2 = it->second;
            if (info1.ac != info2.ac || info1.dc != info2.dc) {
                Diff d;
                d.settingGuid = guid;
                d.settingName = info1.settingName;
                d.subgroupName = info1.subgroupName;
                d.ac1 = info1.ac;
                d.dc1 = info1.dc;
                d.ac2 = info2.ac;
                d.dc2 = info2.dc;
                diffs.push_back(d);
            }
        }
    }

    if (format_ == OutputFormat::Json) {
        std::cout << "{\n  \"scheme1\": {\"guid\": \"" << jsonEscape(scheme1->guid.toString())
                  << "\", \"name\": \"" << jsonEscape(scheme1->name) << "\"},\n"
                  << "  \"scheme2\": {\"guid\": \"" << jsonEscape(scheme2->guid.toString())
                  << "\", \"name\": \"" << jsonEscape(scheme2->name) << "\"},\n"
                  << "  \"total_settings_scheme1\": " << map1.size() << ",\n"
                  << "  \"total_settings_scheme2\": " << map2.size() << ",\n"
                  << "  \"differences\": " << diffs.size() << ",\n"
                  << "  \"diffs\": [\n";
        for (size_t i = 0; i < diffs.size(); ++i) {
            const auto& d = diffs[i];
            std::cout << "    {\"guid\": \"" << jsonEscape(d.settingGuid)
                      << "\", \"name\": \"" << jsonEscape(d.settingName)
                      << "\", \"subgroup\": \"" << jsonEscape(d.subgroupName)
                      << "\", \"ac1\": " << d.ac1 << ", \"dc1\": " << d.dc1
                      << ", \"ac2\": " << d.ac2 << ", \"dc2\": " << d.dc2 << "}";
            if (i + 1 < diffs.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ]\n}" << std::endl;
    } else if (format_ == OutputFormat::Csv) {
        std::cout << "setting_guid,setting_name,subgroup,ac_scheme1,dc_scheme1,ac_scheme2,dc_scheme2" << std::endl;
        for (const auto& d : diffs) {
            std::cout << d.settingGuid << ",\""
                      << jsonEscape(d.settingName) << "\",\""
                      << jsonEscape(d.subgroupName) << "\","
                      << d.ac1 << "," << d.dc1 << ","
                      << d.ac2 << "," << d.dc2 << std::endl;
        }
    } else {
        std::cout << "对比: " << scheme1->name << " vs " << scheme2->name << std::endl;
        std::cout << "  方案1 设置数: " << map1.size() << std::endl;
        std::cout << "  方案2 设置数: " << map2.size() << std::endl;
        std::cout << "  差异数: " << diffs.size() << std::endl;

        if (!diffs.empty()) {
            std::cout << "\n差异项:" << std::endl;
            for (const auto& d : diffs) {
                std::cout << "  " << d.settingGuid << "  " << d.settingName << std::endl;
                std::cout << "    [" << scheme1->name << "] AC=" << d.ac1 << " DC=" << d.dc1 << std::endl;
                std::cout << "    [" << scheme2->name << "] AC=" << d.ac2 << " DC=" << d.dc2 << std::endl;
            }
        } else {
            std::cout << "\n两个方案的设置完全相同。" << std::endl;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// addESportsMode
// ---------------------------------------------------------------------------
int CliApp::cmdESportsMode(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;

    // eSports Mode settings: {setting-GUID, AC-value}
    struct SettingOverride {
        asul::Guid guid;
        uint32_t   acValue;
        const char* name;  // for display
    };

    SettingOverride overrides[] = {
        {{0x4D2B0152, 0x7D5C, 0x498B, {0x88, 0xE2, 0x34, 0x34, 0x53, 0x92, 0xA2, 0xC5}}, 1,   "处理器性能时间检查间隔"},
        {{0x4B92D758, 0x5A24, 0x4851, {0xA4, 0x70, 0x81, 0x5D, 0x78, 0xAE, 0xE1, 0x19}}, 0,   "处理器闲置降级阈值"},
        {{0xD639518A, 0xE56D, 0x4345, {0x8A, 0xF2, 0xB9, 0xF3, 0x2F, 0xB2, 0x61, 0x09}}, 0,   "Primary NVMe Idle Timeout"},
        {{0xD3D55EFD, 0xC1FF, 0x424E, {0x9D, 0xC3, 0x44, 0x1B, 0xE7, 0x83, 0x30, 0x10}}, 0,   "Secondary NVMe Idle Timeout"},
        {{0x48E6B7A6, 0x50F5, 0x4782, {0xA5, 0xD4, 0x53, 0xBB, 0x8F, 0x07, 0xE2, 0x26}}, 0,   "USB 选择性暂停设置"},
        {{0x0853A681, 0x27C8, 0x4100, {0xA2, 0xFD, 0x82, 0x01, 0x3E, 0x97, 0x06, 0x83}}, 0,   "Hub Selective Suspend Timeout"},
        {{0xD4E98F31, 0x5FFE, 0x4CE1, {0xBE, 0x31, 0x1B, 0x38, 0xB3, 0x84, 0xC0, 0x09}}, 0,   "USB 3 Link Power Mangement"},
        {{0xBD3B718A, 0x0680, 0x4D9D, {0x8A, 0xB2, 0xE1, 0xD2, 0xB4, 0xAC, 0x80, 0x6D}}, 0,   "允许唤醒计时器"},
        {{0xD502F7EE, 0x1DC7, 0x4EFD, {0xA5, 0x5D, 0xF0, 0x4B, 0x6F, 0x5C, 0x05, 0x45}}, 0,   "已启用/禁用深度睡眠"},
        {{0x3C0BC021, 0xC8A8, 0x4E07, {0xA9, 0x73, 0x6B, 0x14, 0xCB, 0xCB, 0x2B, 0x7E}}, 0,   "在此时间后关闭显示"},
        {{0x5CA83367, 0x6E45, 0x459F, {0xA2, 0x7B, 0x47, 0x6B, 0x1D, 0x01, 0xC9, 0x36}}, 0,   "合上盖子操作"},
        {{0x465E1F50, 0xB610, 0x473A, {0xAB, 0x58, 0x00, 0xD1, 0x07, 0x7D, 0xC4, 0x18}}, 3,   "处理器性能提升策略"},
        {{0x465E1F50, 0xB610, 0x473A, {0xAB, 0x58, 0x00, 0xD1, 0x07, 0x7D, 0xC4, 0x19}}, 3,   "针对第 1 类处理器电源效率的处理器性能提升策略"},
        {{0x71021B41, 0xC749, 0x4D21, {0xBE, 0x74, 0xA0, 0x0F, 0x33, 0x5D, 0x58, 0x2B}}, 1,   "处理器性能核心放置减小策略"},
        {{0xC7BE0679, 0x2817, 0x4D69, {0x9D, 0x02, 0x51, 0x9A, 0x53, 0x7E, 0xD0, 0xC6}}, 1,   "处理器性能核心放置增加策略"},
        {{0xF735A673, 0x2066, 0x4F80, {0xA0, 0xC5, 0xDD, 0xEE, 0x0C, 0xF1, 0xBF, 0x5D}}, 5,   "处理器性能内核休止并发空间阈值"},
    };
    const int overrideCount = sizeof(overrides) / sizeof(overrides[0]);

    // Step 1: Duplicate Ultimate Performance scheme
    std::cout << "正在基于\"卓越性能\"创建电竞模式方案..." << std::endl;
    auto newGuid = mgr.duplicateScheme(asul::PowerSchemeGuids::ULTIMATE_PERFORMANCE, ec);
    if (ec) {
        std::cerr << "复制方案失败: " << ec.message() << std::endl;
        return 1;
    }

    // Step 2: Rename to "eSports Mode"
    if (!mgr.renameScheme(newGuid, "eSports Mode", ec)) {
        std::cerr << "重命名方案失败: " << ec.message() << std::endl;
        mgr.deleteScheme(newGuid, ec);
        return 1;
    }

    // Step 2b: Write description
    std::cout << "正在写入方案描述..." << std::endl;
    if (!mgr.writeSchemeDescription(newGuid, "由 AsulPowerManager 生成", ec)) {
        std::cerr << "写入方案描述失败: " << ec.message() << " (错误码: " << ec.value() << ")" << std::endl;
        // 不删除方案，继续执行
    } else {
        std::cout << "方案描述已写入" << std::endl;
    }

    // Step 3: Enumerate all subgroups to build setting->subgroup mapping
    auto subgroups = mgr.enumerateSubgroups(newGuid, ec);
    if (ec) {
        std::cerr << "枚举子组失败: " << ec.message() << std::endl;
        mgr.deleteScheme(newGuid, ec);
        return 1;
    }

    // Build a map: setting GUID -> subgroup GUID (by scanning all subgroups)
    struct SettingLocation {
        asul::Guid subgroupGuid;
        std::string settingName;
    };
    std::map<std::string, SettingLocation> settingMap;

    for (const auto& sg : subgroups) {
        auto settings = mgr.enumerateSettings(newGuid, sg.guid, ec);
        if (ec) continue;
        for (const auto& s : settings) {
            settingMap[s.guid.toString()] = {sg.guid, s.name};
        }
    }

    // Step 4: Apply overrides
    std::cout << "正在应用电竞模式设置..." << std::endl;
    int applied = 0, skipped = 0;

    for (int i = 0; i < overrideCount; ++i) {
        const auto& ov = overrides[i];
        auto it = settingMap.find(ov.guid.toString());

        if (it == settingMap.end()) {
            std::cerr << "  [跳过] " << ov.guid.toString()
                      << " (" << ov.name << ") - 当前系统不支持此设置" << std::endl;
            ++skipped;
            continue;
        }

        auto& loc = it->second;
        if (!mgr.writeACValueIndex(newGuid, loc.subgroupGuid, ov.guid, ov.acValue, ec)) {
            std::cerr << "  [失败] " << loc.settingName
                      << " AC=" << ov.acValue << " - " << ec.message() << std::endl;
            ++skipped;
            continue;
        }

        std::cout << "  [完成] " << loc.settingName
                  << " AC=" << ov.acValue << std::endl;
        ++applied;
    }

    // Summary
    if (format_ == OutputFormat::Json) {
        std::cout << "{\"guid\": \"" << jsonEscape(newGuid.toString())
                  << "\", \"name\": \"eSports Mode\", \"applied\": " << applied
                  << ", \"skipped\": " << skipped << "}" << std::endl;
    } else {
        std::cout << "\n电竞模式方案已创建!" << std::endl;
        std::cout << "  GUID: " << newGuid.toString() << std::endl;
        std::cout << "  已应用: " << applied << " 项" << std::endl;
        if (skipped > 0) {
            std::cout << "  跳过: " << skipped << " 项 (当前系统不支持或写入失败)" << std::endl;
        }
    }
    return 0;
}

} // namespace asul
