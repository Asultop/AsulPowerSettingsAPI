#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace asul {

// GUID wrapper
struct Guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];

    std::string toString() const;
    static Guid fromString(const std::string& str, bool& ok);
    bool operator==(const Guid& rhs) const;
    bool operator!=(const Guid& rhs) const { return !(*this == rhs); }
    bool operator<(const Guid& rhs) const;
};

// Well-known Windows power scheme GUIDs (fixed across all Windows installations)
namespace PowerSchemeGuids {
    // 平衡 (Balanced)
    static const Guid BALANCED             = {0x381B4222, 0xF694, 0x41F0, {0x96, 0x85, 0xFF, 0x5B, 0xB2, 0x60, 0xDF, 0x2E}};
    // 高性能 (High Performance)
    static const Guid HIGH_PERFORMANCE     = {0x8C5E7FDA, 0xE8BF, 0x4A96, {0x9A, 0x85, 0xA6, 0xE2, 0x3A, 0x8C, 0x63, 0x5C}};
    // 节能 (Power Saver)
    static const Guid POWER_SAVER          = {0xA1841308, 0x3541, 0x4FAB, {0xBC, 0x81, 0xF7, 0x15, 0x56, 0xF2, 0x0B, 0x4A}};
    // 卓越性能 (Ultimate Performance)
    static const Guid ULTIMATE_PERFORMANCE = {0xE9A42B02, 0xD5DF, 0x448D, {0xAA, 0x00, 0x03, 0xF1, 0x47, 0x49, 0xEB, 0x61}};
}

// Well-known Windows power subgroup GUIDs
namespace PowerSubgroupGuids {
    // 睡眠 (Sleep)
    static const Guid SLEEP              = {0x238C9FA8, 0x0AAD, 0x41ED, {0x83, 0xF4, 0x97, 0xBE, 0x24, 0x2C, 0x8F, 0x20}};
    // 显示 (Display)
    static const Guid DISPLAY            = {0x7516B95F, 0xF776, 0x4464, {0x8C, 0x53, 0x06, 0x16, 0x7F, 0x40, 0xCC, 0x99}};
    // 硬盘 (Hard Disk)
    static const Guid HARD_DISK          = {0x0012EE47, 0x9041, 0x4B5D, {0x9B, 0x77, 0x53, 0x5F, 0xBA, 0x8B, 0x14, 0x42}};
    // 处理器电源管理 (Processor Power Management)
    static const Guid PROCESSOR          = {0x54533251, 0x82BE, 0x4824, {0x96, 0xC1, 0x47, 0xB6, 0x0B, 0x74, 0x0D, 0x00}};
    // 无线适配器设置 (Wireless Adapter Settings)
    static const Guid WIRELESS_ADAPTER   = {0x19CBB8FA, 0x5279, 0x450E, {0x9F, 0xAC, 0x8A, 0x3D, 0x5F, 0xED, 0xD0, 0xC1}};
    // USB 设置 (USB Settings)
    static const Guid USB                = {0x2A737441, 0x1930, 0x4402, {0x8D, 0x77, 0xB2, 0xBB, 0xA3, 0x0C, 0x1A, 0x22}};
    // Internet Explorer
    static const Guid INTERNET_EXPLORER  = {0x02F815B5, 0xA5CF, 0x4C84, {0xBF, 0x20, 0x64, 0x9D, 0x1F, 0x75, 0xD3, 0xD8}};
    // 桌面背景设置 (Desktop Background Settings)
    static const Guid DESKTOP_BACKGROUND = {0x0D7DBAE2, 0x4294, 0x402A, {0xBA, 0x8E, 0x26, 0x77, 0x7E, 0x84, 0x88, 0xCD}};
}

// Well-known Windows power setting GUIDs
namespace PowerSettingGuids {
    // 在此时间后关闭显示器 (Turn off display after)
    static const Guid TURNOFF_DISPLAY    = {0x3C0BC021, 0xC8A8, 0x4E07, {0xA9, 0x73, 0x6B, 0x14, 0xCB, 0xCB, 0x2B, 0x7E}};
    // 在此时间后睡眠 (Sleep after)
    static const Guid SLEEP_AFTER        = {0x29F6C1DB, 0x86DA, 0x48C5, {0x9F, 0xDB, 0xF2, 0xB6, 0x7B, 0x1F, 0x44, 0xDA}};
    // 在此时间后休眠 (Hibernate after)
    static const Guid HIBERNATE_AFTER    = {0x9D7815A6, 0x7EE4, 0x497E, {0x88, 0x88, 0x51, 0x5A, 0x05, 0xF0, 0x23, 0x64}};
    // 允许混合睡眠 (Allow hybrid sleep)
    static const Guid ALLOW_HYBRID_SLEEP = {0x94AC6D29, 0x73CE, 0x41A6, {0x80, 0x9F, 0x63, 0x63, 0xBA, 0x21, 0xB4, 0x7E}};
    // 允许唤醒计时器 (Allow wake timers)
    static const Guid ALLOW_WAKE_TIMERS  = {0xBD3B718A, 0x0680, 0x4D9D, {0x8A, 0xB2, 0xE1, 0xD2, 0xB4, 0xAC, 0x80, 0x6D}};
    // 最小处理器状态 (Minimum processor state)
    static const Guid MIN_PROCESSOR_STATE = {0x893DEE8E, 0x2BEF, 0x41E0, {0x89, 0xDC, 0x9B, 0x5D, 0x67, 0xD3, 0x63, 0x0F}};
    // 最大处理器状态 (Maximum processor state)
    static const Guid MAX_PROCESSOR_STATE = {0xBC5038F7, 0x23E0, 0x4960, {0x96, 0xDA, 0x33, 0xAB, 0xAF, 0x59, 0x35, 0xEC}};
}

// A single power setting (e.g. "Turn off display after")
struct PowerSetting {
    Guid        guid;
    std::string name;
    uint32_t    attributes;  // 0 = hidden by Windows, 1 = visible
    std::vector<uint32_t> acValues;   // possible AC value indices
    std::vector<uint32_t> dcValues;   // possible DC value indices
    uint32_t    acDefault;
    uint32_t    dcDefault;
    uint32_t    acValueOverride;  // scheme-level override (UINT_MAX if none)
    uint32_t    dcValueOverride;
    bool        isHidden() const { return (attributes & 1) == 0; }
};

// A subgroup within a power scheme (e.g. "Display", "Sleep")
struct PowerSubgroup {
    Guid        guid;
    std::string name;
    uint32_t    attributes;
    std::vector<PowerSetting> settings;
    bool        isHidden() const { return (attributes & 1) == 0; }
};

// A complete power scheme (e.g. "Balanced", "High performance")
struct PowerScheme {
    Guid        guid;
    std::string name;
    std::vector<PowerSubgroup> subgroups;
};

// Value description for a setting
struct SettingValue {
    uint32_t    index;
    std::string name;
};

} // namespace asul
