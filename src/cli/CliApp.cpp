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

namespace asul {

CliApp::CliApp() = default;

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
  help                          显示此帮助信息
  version                       显示版本
)" << std::endl;
}

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
    if (cmd == "list")        return cmdList(argc, argv);
    if (cmd == "active")      return cmdActive(argc, argv);
    if (cmd == "set-active")  return cmdSetActive(argc, argv);
    if (cmd == "subgroups")   return cmdSubgroups(argc, argv);
    if (cmd == "settings")    return cmdSettings(argc, argv);
    if (cmd == "get")         return cmdGet(argc, argv);
    if (cmd == "set")         return cmdSet(argc, argv);
    if (cmd == "hidden")      return cmdHidden(argc, argv);
    if (cmd == "scan")        return cmdScan(argc, argv);

    std::cerr << "未知命令: " << cmd << std::endl;
    printUsage(argv[0]);
    return 1;
}

int CliApp::cmdList(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    if (ec) {
        std::cerr << "枚举电源方案失败: " << ec.message() << std::endl;
        return 1;
    }
    std::cout << "电源方案 (" << schemes.size() << " 个):" << std::endl;
    for (const auto& s : schemes) {
        std::cout << "  " << s.guid.toString() << "  " << s.name << std::endl;
    }
    return 0;
}

int CliApp::cmdActive(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    auto guid = mgr.getActiveScheme(ec);
    if (ec) {
        std::cerr << "错误: " << ec.message() << std::endl;
        return 1;
    }
    std::string name = mgr.readFriendlyName(guid, ec);
    std::cout << "当前活动方案: " << guid.toString();
    if (!name.empty()) std::cout << "  (" << name << ")";
    std::cout << std::endl;
    return 0;
}

int CliApp::cmdSetActive(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings set-active <GUID>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    GUID g;
    std::wstring wguid(guidStr.begin(), guidStr.end());
    if (CLSIDFromString(wguid.c_str(), &g) != S_OK) {
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
    std::cout << "已切换活动方案为 " << schemeGuid.toString() << std::endl;
    return 0;
}

int CliApp::cmdSubgroups(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: power-settings subgroups <方案-GUID>" << std::endl;
        return 1;
    }
    std::string guidStr = argv[2];
    GUID g;
    std::wstring wguid(guidStr.begin(), guidStr.end());
    if (CLSIDFromString(wguid.c_str(), &g) != S_OK) {
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
    std::cout << "子组 (" << subgroups.size() << " 个):" << std::endl;
    for (const auto& sg : subgroups) {
        std::string hidden = sg.isHidden() ? " [隐藏]" : "";
        std::cout << "  " << sg.guid.toString() << "  " << sg.name << hidden << std::endl;
    }
    return 0;
}

int CliApp::cmdSettings(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: power-settings settings <方案-GUID> <子组-GUID>" << std::endl;
        return 1;
    }
    std::string schemeStr = argv[2];
    std::string subStr = argv[3];

    GUID schemeG, subG;
    std::wstring ws(schemeStr.begin(), schemeStr.end());
    std::wstring wsub(subStr.begin(), subStr.end());
    if (CLSIDFromString(ws.c_str(), &schemeG) != S_OK ||
        CLSIDFromString(wsub.c_str(), &subG) != S_OK) {
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
    std::cout << "设置项 (" << settings.size() << " 个):" << std::endl;
    for (const auto& s : settings) {
        std::string hidden = s.isHidden() ? " [隐藏]" : "";
        std::cout << "  " << s.guid.toString() << "  " << s.name << hidden << std::endl;
        std::cout << "    AC 默认=" << s.acDefault
                  << "  DC 默认=" << s.dcDefault << std::endl;
    }
    return 0;
}

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
    std::wstring ws(schemeStr.begin(), schemeStr.end());
    std::wstring wsub(subStr.begin(), subStr.end());
    std::wstring wset(setStr.begin(), setStr.end());
    if (CLSIDFromString(ws.c_str(), &schemeG) != S_OK ||
        CLSIDFromString(wsub.c_str(), &subG) != S_OK ||
        CLSIDFromString(wset.c_str(), &setG) != S_OK) {
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

    std::cout << "设置项: " << setStr << std::endl;
    std::cout << "  AC 值: " << (acOk ? std::to_string(acVal) : "不可用") << std::endl;
    std::cout << "  DC 值: " << (dcOk ? std::to_string(dcVal) : "不可用") << std::endl;
    return 0;
}

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
    std::wstring ws(schemeStr.begin(), schemeStr.end());
    std::wstring wsub(subStr.begin(), subStr.end());
    std::wstring wset(setStr.begin(), setStr.end());
    if (CLSIDFromString(ws.c_str(), &schemeG) != S_OK ||
        CLSIDFromString(wsub.c_str(), &subG) != S_OK ||
        CLSIDFromString(wset.c_str(), &setG) != S_OK) {
        std::cerr << "无效的 GUID" << std::endl;
        return 1;
    }

    PowerSettingsManager mgr;
    std::error_code ec;
    auto sg = fromWindowsGuid(schemeG);
    auto sub = fromWindowsGuid(subG);
    auto set = fromWindowsGuid(setG);

    int rc = 0;
    if (acVal != UINT32_MAX) {
        if (!mgr.writeACValueIndex(sg, sub, set, acVal, ec)) {
            std::cerr << "写入 AC 值失败: " << ec.message() << std::endl;
            rc = 1;
        } else {
            std::cout << "AC 值已设置为 " << acVal << std::endl;
        }
    }
    if (dcVal != UINT32_MAX) {
        if (!mgr.writeDCValueIndex(sg, sub, set, dcVal, ec)) {
            std::cerr << "写入 DC 值失败: " << ec.message() << std::endl;
            rc = 1;
        } else {
            std::cout << "DC 值已设置为 " << dcVal << std::endl;
        }
    }
    if (acVal == UINT32_MAX && dcVal == UINT32_MAX) {
        std::cerr << "未指定要设置的值，请使用 --ac <值> 和/或 --dc <值>" << std::endl;
        return 1;
    }
    return rc;
}

int CliApp::cmdHidden(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    auto hidden = mgr.enumerateHiddenSettings(ec);
    if (ec) {
        std::cerr << "读取注册表失败: " << ec.message() << std::endl;
        return 1;
    }
    std::cout << "被隐藏的电源设置 (" << hidden.size() << " 个):" << std::endl;
    for (const auto& s : hidden) {
        std::cout << "  " << s.guid.toString() << "  " << s.name << std::endl;
    }
    return 0;
}

int CliApp::cmdScan(int /*argc*/, char* /*argv*/[]) {
    PowerSettingsManager mgr;
    std::error_code ec;
    std::cout << "正在执行完整电源设置扫描..." << std::endl;
    auto schemes = mgr.fullScan(ec);
    if (ec) {
        std::cerr << "扫描出错: " << ec.message() << std::endl;
        return 1;
    }

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
    return 0;
}

} // namespace asul
