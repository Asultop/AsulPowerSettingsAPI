#include <AsulPowerSettingsApi/PowerSettingsManager.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <objbase.h>
#include <PowrProf.h>
#include <cstring>
#include <memory>

#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")

namespace asul {

// ---------------------------------------------------------------------------
// GUID conversion helpers
// ---------------------------------------------------------------------------
Guid fromWindowsGuid(const GUID& g) {
    Guid r;
    r.data1 = g.Data1;
    r.data2 = g.Data2;
    r.data3 = g.Data3;
    std::memcpy(r.data4, g.Data4, 8);
    return r;
}

GUID toWindowsGuid(const Guid& g) {
    GUID r;
    r.Data1 = g.data1;
    r.Data2 = g.data2;
    r.Data3 = g.data3;
    std::memcpy(r.Data4, g.data4, 8);
    return r;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

std::string lastErrorMessage() {
    DWORD err = GetLastError();
    if (err == 0) return {};
    LPSTR buf = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, err, 0, reinterpret_cast<LPSTR>(&buf), 0, nullptr);
    std::string msg = buf ? buf : "unknown error";
    LocalFree(buf);
    return msg;
}

// Read a friendly name via PowerReadFriendlyName (works for scheme/subgroup/setting)
std::string readName(HKEY root, const GUID* schemeGuid, const GUID* subgroupGuid,
                     const GUID* settingGuid, std::error_code& ec) {
    ec.clear();
    DWORD size = 0;
    DWORD rc = PowerReadFriendlyName(root, schemeGuid, subgroupGuid, settingGuid,
                                     nullptr, &size);
    if (rc != ERROR_MORE_DATA && rc != ERROR_SUCCESS) {
        if (rc != 0) ec = std::error_code(static_cast<int>(rc), std::system_category());
        return {};
    }
    if (size == 0) return {};

    auto buf = std::unique_ptr<BYTE[]>(new BYTE[size]);
    rc = PowerReadFriendlyName(root, schemeGuid, subgroupGuid, settingGuid,
                               buf.get(), &size);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return {};
    }
    // The name is a UTF-16 string
    const wchar_t* wstr = reinterpret_cast<const wchar_t*>(buf.get());
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(static_cast<size_t>(len) - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
    return result;
}

// Read DWORD attribute from registry for a power setting/subgroup
// Attributes are stored under HKLM\SYSTEM\CurrentControlSet\Control\Power\PowerSettings\{sub}\{set}\Attributes
DWORD readAttributes(const GUID* subgroupGuid, const GUID* settingGuid) {
    if (!subgroupGuid) return 0;

    std::string subStr(40, '\0');
    int n = snprintf(&subStr[0], subStr.size(),
        "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        subgroupGuid->Data1, subgroupGuid->Data2, subgroupGuid->Data3,
        subgroupGuid->Data4[0], subgroupGuid->Data4[1], subgroupGuid->Data4[2],
        subgroupGuid->Data4[3], subgroupGuid->Data4[4], subgroupGuid->Data4[5],
        subgroupGuid->Data4[6], subgroupGuid->Data4[7]);
    subStr.resize(n);

    std::string regPath = "SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\" + subStr;
    if (settingGuid) {
        std::string setStr(40, '\0');
        int m = snprintf(&setStr[0], setStr.size(),
            "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            settingGuid->Data1, settingGuid->Data2, settingGuid->Data3,
            settingGuid->Data4[0], settingGuid->Data4[1], settingGuid->Data4[2],
            settingGuid->Data4[3], settingGuid->Data4[4], settingGuid->Data4[5],
            settingGuid->Data4[6], settingGuid->Data4[7]);
        setStr.resize(m);
        regPath += "\\" + setStr;
    }

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return 0;

    DWORD attrs = 0;
    DWORD size = sizeof(attrs);
    DWORD type = 0;
    RegQueryValueExA(hKey, "Attributes", nullptr, &type, reinterpret_cast<BYTE*>(&attrs), &size);
    RegCloseKey(hKey);
    return attrs;
}

// Strip braces from GUID string for registry paths
std::string stripBraces(const Guid& g) {
    std::string s = g.toString();
    if (!s.empty() && s.front() == '{') s.erase(0, 1);
    if (!s.empty() && s.back() == '}') s.pop_back();
    return s;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// PowerSettingsManager
// ---------------------------------------------------------------------------
PowerSettingsManager::PowerSettingsManager() = default;
PowerSettingsManager::~PowerSettingsManager() = default;

// --- Scheme enumeration ---
std::vector<PowerScheme> PowerSettingsManager::enumerateSchemes(std::error_code& ec) const {
    ec.clear();
    std::vector<PowerScheme> schemes;
    GUID guid;
    DWORD bufferSize = sizeof(GUID);

    for (DWORD index = 0; ; ++index) {
        bufferSize = sizeof(GUID);
        DWORD rc = PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME,
                                  index, reinterpret_cast<BYTE*>(&guid), &bufferSize);
        if (rc == ERROR_NO_MORE_ITEMS) break;
        if (rc != ERROR_SUCCESS) {
            ec = std::error_code(static_cast<int>(rc), std::system_category());
            break;
        }
        PowerScheme scheme;
        scheme.guid = fromWindowsGuid(guid);
        scheme.name = readName(nullptr, &guid, nullptr, nullptr, ec);
        schemes.push_back(std::move(scheme));
    }
    return schemes;
}

// --- Active scheme ---
Guid PowerSettingsManager::getActiveScheme(std::error_code& ec) const {
    ec.clear();
    GUID* guidPtr = nullptr;
    DWORD rc = PowerGetActiveScheme(nullptr, &guidPtr);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return {};
    }
    Guid result = fromWindowsGuid(*guidPtr);
    LocalFree(guidPtr);
    return result;
}

bool PowerSettingsManager::setActiveScheme(const Guid& schemeGuid, std::error_code& ec) const {
    ec.clear();
    GUID g = toWindowsGuid(schemeGuid);
    DWORD rc = PowerSetActiveScheme(nullptr, &g);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return false;
    }
    return true;
}

bool PowerSettingsManager::restoreDefaultScheme(const Guid& /*schemeGuid*/, std::error_code& ec) const {
    ec.clear();
    // PowerRestoreDefaultPowerSchemes restores ALL schemes
    DWORD rc = PowerRestoreDefaultPowerSchemes();
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return false;
    }
    return true;
}

// --- Subgroup enumeration ---
std::vector<PowerSubgroup> PowerSettingsManager::enumerateSubgroups(
    const Guid& schemeGuid, std::error_code& ec) const {
    ec.clear();
    std::vector<PowerSubgroup> subgroups;
    GUID sgGuid;
    GUID schemeG = toWindowsGuid(schemeGuid);

    for (DWORD index = 0; ; ++index) {
        DWORD bufferSize = sizeof(GUID);
        DWORD rc = PowerEnumerate(nullptr, &schemeG, nullptr, ACCESS_SUBGROUP,
                                  index, reinterpret_cast<BYTE*>(&sgGuid), &bufferSize);
        if (rc == ERROR_NO_MORE_ITEMS) break;
        if (rc != ERROR_SUCCESS) {
            ec = std::error_code(static_cast<int>(rc), std::system_category());
            break;
        }
        PowerSubgroup sg;
        sg.guid = fromWindowsGuid(sgGuid);
        sg.name = readName(nullptr, &schemeG, &sgGuid, nullptr, ec);
        sg.attributes = readAttributes(&sgGuid, nullptr);
        subgroups.push_back(std::move(sg));
    }
    return subgroups;
}

// --- Setting enumeration ---
std::vector<PowerSetting> PowerSettingsManager::enumerateSettings(
    const Guid& schemeGuid, const Guid& subgroupGuid, std::error_code& ec) const {
    ec.clear();
    std::vector<PowerSetting> settings;
    GUID setGuid;
    GUID schemeG = toWindowsGuid(schemeGuid);
    GUID sgG = toWindowsGuid(subgroupGuid);

    for (DWORD index = 0; ; ++index) {
        DWORD bufferSize = sizeof(GUID);
        DWORD rc = PowerEnumerate(nullptr, &schemeG, &sgG, ACCESS_INDIVIDUAL_SETTING,
                                  index, reinterpret_cast<BYTE*>(&setGuid), &bufferSize);
        if (rc == ERROR_NO_MORE_ITEMS) break;
        if (rc != ERROR_SUCCESS) {
            ec = std::error_code(static_cast<int>(rc), std::system_category());
            break;
        }
        PowerSetting setting;
        setting.guid = fromWindowsGuid(setGuid);
        setting.name = readName(nullptr, &schemeG, &sgG, &setGuid, ec);
        setting.attributes = readAttributes(&sgG, &setGuid);

        // Read current values
        uint32_t acVal = 0, dcVal = 0;
        std::error_code tmpEc;
        if (readACValueIndex(schemeGuid, subgroupGuid, setting.guid, acVal, tmpEc)) {
            setting.acValueOverride = acVal;
        } else {
            setting.acValueOverride = UINT32_MAX;
        }
        if (readDCValueIndex(schemeGuid, subgroupGuid, setting.guid, dcVal, tmpEc)) {
            setting.dcValueOverride = dcVal;
        } else {
            setting.dcValueOverride = UINT32_MAX;
        }

        // Defaults not available via API; set to 0
        setting.acDefault = 0;
        setting.dcDefault = 0;

        settings.push_back(std::move(setting));
    }
    return settings;
}

// --- Friendly name ---
std::string PowerSettingsManager::readFriendlyName(const Guid& guid,
                                                    std::error_code& ec) const {
    GUID g = toWindowsGuid(guid);
    return readName(nullptr, &g, nullptr, nullptr, ec);
}

// --- Value read/write ---
bool PowerSettingsManager::readACValueIndex(
    const Guid& schemeGuid, const Guid& subgroupGuid,
    const Guid& settingGuid, uint32_t& valueIndex, std::error_code& ec) const {
    ec.clear();
    GUID sg = toWindowsGuid(schemeGuid);
    GUID sub = toWindowsGuid(subgroupGuid);
    GUID set = toWindowsGuid(settingGuid);
    DWORD val = 0;
    DWORD rc = PowerReadACValueIndex(nullptr, &sg, &sub, &set, &val);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return false;
    }
    valueIndex = val;
    return true;
}

bool PowerSettingsManager::readDCValueIndex(
    const Guid& schemeGuid, const Guid& subgroupGuid,
    const Guid& settingGuid, uint32_t& valueIndex, std::error_code& ec) const {
    ec.clear();
    GUID sg = toWindowsGuid(schemeGuid);
    GUID sub = toWindowsGuid(subgroupGuid);
    GUID set = toWindowsGuid(settingGuid);
    DWORD val = 0;
    DWORD rc = PowerReadDCValueIndex(nullptr, &sg, &sub, &set, &val);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return false;
    }
    valueIndex = val;
    return true;
}

bool PowerSettingsManager::writeACValueIndex(
    const Guid& schemeGuid, const Guid& subgroupGuid,
    const Guid& settingGuid, uint32_t valueIndex, std::error_code& ec) const {
    ec.clear();
    GUID sg = toWindowsGuid(schemeGuid);
    GUID sub = toWindowsGuid(subgroupGuid);
    GUID set = toWindowsGuid(settingGuid);
    DWORD rc = PowerWriteACValueIndex(nullptr, &sg, &sub, &set, valueIndex);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return false;
    }
    return true;
}

bool PowerSettingsManager::writeDCValueIndex(
    const Guid& schemeGuid, const Guid& subgroupGuid,
    const Guid& settingGuid, uint32_t valueIndex, std::error_code& ec) const {
    ec.clear();
    GUID sg = toWindowsGuid(schemeGuid);
    GUID sub = toWindowsGuid(subgroupGuid);
    GUID set = toWindowsGuid(settingGuid);
    DWORD rc = PowerWriteDCValueIndex(nullptr, &sg, &sub, &set, valueIndex);
    if (rc != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(rc), std::system_category());
        return false;
    }
    return true;
}

// --- Possible values ---
std::vector<SettingValue> PowerSettingsManager::readPossibleValues(
    const Guid& subgroupGuid, const Guid& settingGuid,
    std::error_code& ec) const {
    ec.clear();
    std::vector<SettingValue> values;

    std::string regPath = "SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\"
        + stripBraces(subgroupGuid) + "\\" + stripBraces(settingGuid) + "\\PossibleValues";

    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey);
    if (res == ERROR_SUCCESS) {
        char valueName[256];
        DWORD valueNameLen;
        DWORD valueData;
        DWORD valueDataLen;
        DWORD type;

        for (DWORD i = 0; ; ++i) {
            valueNameLen = sizeof(valueName);
            valueDataLen = sizeof(valueData);
            res = RegEnumValueA(hKey, i, valueName, &valueNameLen, nullptr,
                               &type, reinterpret_cast<BYTE*>(&valueData), &valueDataLen);
            if (res == ERROR_NO_MORE_ITEMS) break;
            if (res != ERROR_SUCCESS) continue;
            if (type == REG_DWORD) {
                SettingValue sv;
                sv.index = valueData;
                sv.name = std::string(valueName, valueNameLen);
                values.push_back(std::move(sv));
            }
        }
        RegCloseKey(hKey);
    }

    return values;
}

// --- Full scan ---
std::vector<PowerScheme> PowerSettingsManager::fullScan(std::error_code& ec) const {
    auto schemes = enumerateSchemes(ec);
    if (ec) return schemes;

    for (auto& scheme : schemes) {
        scheme.subgroups = enumerateSubgroups(scheme.guid, ec);
        if (ec) return schemes;
        for (auto& subgroup : scheme.subgroups) {
            subgroup.settings = enumerateSettings(scheme.guid, subgroup.guid, ec);
            if (ec) return schemes;
        }
    }
    return schemes;
}

// --- Hidden settings via registry ---
std::vector<PowerSetting> PowerSettingsManager::enumerateHiddenSettings(
    std::error_code& ec) const {
    ec.clear();
    std::vector<PowerSetting> hidden;

    const char* regPath = "SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings";
    HKEY hRoot;
    LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hRoot);
    if (res != ERROR_SUCCESS) {
        ec = std::error_code(static_cast<int>(res), std::system_category());
        return hidden;
    }

    char subName[64];
    for (DWORD si = 0; ; ++si) {
        DWORD subNameLen = sizeof(subName);
        res = RegEnumKeyExA(hRoot, si, subName, &subNameLen, nullptr, nullptr, nullptr, nullptr);
        if (res == ERROR_NO_MORE_ITEMS) break;
        if (res != ERROR_SUCCESS) continue;

        HKEY hSub;
        if (RegOpenKeyExA(hRoot, subName, 0, KEY_READ, &hSub) != ERROR_SUCCESS)
            continue;

        // Enumerate setting GUIDs under subgroup
        char setName[64];
        for (DWORD ssi = 0; ; ++ssi) {
            DWORD setNameLen = sizeof(setName);
            res = RegEnumKeyExA(hSub, ssi, setName, &setNameLen,
                               nullptr, nullptr, nullptr, nullptr);
            if (res == ERROR_NO_MORE_ITEMS) break;
            if (res != ERROR_SUCCESS) continue;

            HKEY hSet;
            if (RegOpenKeyExA(hSub, setName, 0, KEY_READ, &hSet) != ERROR_SUCCESS)
                continue;

            DWORD settingAttrs = 0;
            DWORD attrSize = sizeof(settingAttrs);
            DWORD attrType = 0;
            RegQueryValueExA(hSet, "Attributes", nullptr, &attrType,
                             reinterpret_cast<BYTE*>(&settingAttrs), &attrSize);

            // Hidden = Attributes bit 0 is 0
            if ((settingAttrs & 1) == 0) {
                PowerSetting ps;
                // Parse GUID from registry key name
                // Wrap in braces for CLSIDFromString
                std::wstring wsetName(setName, setName + setNameLen);
                if (wsetName.front() != L'{') wsetName = L"{" + wsetName + L"}";
                GUID parsed;
                if (CLSIDFromString(wsetName.c_str(), &parsed) == S_OK) {
                    ps.guid = fromWindowsGuid(parsed);
                }
                ps.attributes = settingAttrs;

                // Read friendly name via Power API
                std::error_code nameEc;
                GUID psGuidWin = toWindowsGuid(ps.guid);
                ps.name = readName(nullptr, &psGuidWin, nullptr, nullptr, nameEc);
                if (ps.name.empty()) {
                    // Fallback: read FriendlyName from registry
                    DWORD fnSize = 0;
                    if (RegQueryValueExW(hSet, L"FriendlyName", nullptr, nullptr,
                                         nullptr, &fnSize) == ERROR_SUCCESS && fnSize > 0) {
                        auto fnBuf = std::unique_ptr<BYTE[]>(new BYTE[fnSize]);
                        if (RegQueryValueExW(hSet, L"FriendlyName", nullptr, nullptr,
                                             fnBuf.get(), &fnSize) == ERROR_SUCCESS) {
                            int len = WideCharToMultiByte(CP_UTF8, 0,
                                reinterpret_cast<const wchar_t*>(fnBuf.get()), -1,
                                nullptr, 0, nullptr, nullptr);
                            if (len > 0) {
                                ps.name.resize(static_cast<size_t>(len) - 1);
                                WideCharToMultiByte(CP_UTF8, 0,
                                    reinterpret_cast<const wchar_t*>(fnBuf.get()), -1,
                                    &ps.name[0], len, nullptr, nullptr);
                            }
                        }
                    }
                    if (ps.name.empty()) {
                        ps.name = setName;  // raw GUID as fallback
                    }
                }

                hidden.push_back(std::move(ps));
            }
            RegCloseKey(hSet);
        }
        RegCloseKey(hSub);
    }
    RegCloseKey(hRoot);
    return hidden;
}

} // namespace asul
