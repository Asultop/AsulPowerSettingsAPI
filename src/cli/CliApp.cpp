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
  addESportsMode                添加电竞模式电源方案
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
    if (cmd == "create")      return cmdCreate(argc, argv);
    if (cmd == "duplicate")   return cmdDuplicate(argc, argv);
    if (cmd == "delete")      return cmdDelete(argc, argv);
    if (cmd == "rename")      return cmdRename(argc, argv);
    if (cmd == "import")      return cmdImport(argc, argv);
    if (cmd == "addESportsMode") return cmdESportsMode(argc, argv);

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
    std::cout << "已创建电源方案 \"" << name << "\"" << std::endl;
    std::cout << "  GUID: " << newGuid.toString() << std::endl;
    return 0;
}

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
    std::cout << "已复制电源方案" << std::endl;
    std::cout << "  GUID: " << newGuid.toString();
    if (!name.empty()) std::cout << "  (" << name << ")";
    std::cout << std::endl;
    return 0;
}

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
    std::cout << "已删除电源方案 " << guidStr << std::endl;
    return 0;
}

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
    std::cout << "已将电源方案重命名为 \"" << newName << "\"" << std::endl;
    return 0;
}

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
    std::cout << "已导入电源方案" << std::endl;
    std::cout << "  GUID: " << guid.toString();
    if (!name.empty()) std::cout << "  (" << name << ")";
    std::cout << std::endl;
    return 0;
}

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
    std::cout << "\n电竞模式方案已创建!" << std::endl;
    std::cout << "  GUID: " << newGuid.toString() << std::endl;
    std::cout << "  已应用: " << applied << " 项" << std::endl;
    if (skipped > 0) {
        std::cout << "  跳过: " << skipped << " 项 (当前系统不支持或写入失败)" << std::endl;
    }
    return 0;
}

} // namespace asul
