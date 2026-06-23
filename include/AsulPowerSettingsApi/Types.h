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
    bool operator==(const Guid& rhs) const;
    bool operator!=(const Guid& rhs) const { return !(*this == rhs); }
    bool operator<(const Guid& rhs) const;
};

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
