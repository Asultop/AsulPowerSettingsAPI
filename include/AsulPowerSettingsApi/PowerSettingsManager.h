#pragma once

#include "Types.h"
#include <guiddef.h>
#include <functional>
#include <system_error>

namespace asul {

// Main API class for enumerating and managing Windows power settings
class PowerSettingsManager {
public:
    PowerSettingsManager();
    ~PowerSettingsManager();

    // Non-copyable
    PowerSettingsManager(const PowerSettingsManager&) = delete;
    PowerSettingsManager& operator=(const PowerSettingsManager&) = delete;

    // --- Scheme operations ---

    // Enumerate all power schemes
    std::vector<PowerScheme> enumerateSchemes(std::error_code& ec) const;

    // Get active power scheme GUID
    Guid getActiveScheme(std::error_code& ec) const;

    // Set active power scheme
    bool setActiveScheme(const Guid& schemeGuid, std::error_code& ec) const;

    // Restore default power scheme settings
    bool restoreDefaultScheme(const Guid& schemeGuid, std::error_code& ec) const;

    // Create a new power scheme by duplicating an existing one and renaming it
    // Returns the GUID of the newly created scheme
    Guid createScheme(const Guid& sourceSchemeGuid, const std::string& newName,
                      std::error_code& ec) const;

    // Duplicate an existing power scheme (copy as-is)
    // Returns the GUID of the duplicated scheme
    Guid duplicateScheme(const Guid& sourceSchemeGuid, std::error_code& ec) const;

    // Delete a power scheme
    bool deleteScheme(const Guid& schemeGuid, std::error_code& ec) const;

    // Rename a power scheme
    bool renameScheme(const Guid& schemeGuid, const std::string& newName,
                      std::error_code& ec) const;

    // Write scheme description (subtitle)
    bool writeSchemeDescription(const Guid& schemeGuid, const std::string& description,
                                std::error_code& ec) const;

    // Import a power scheme from a .pow file
    // Returns the GUID of the imported scheme
    Guid importScheme(const std::string& filePath, std::error_code& ec) const;

    // --- Subgroup operations ---

    // Enumerate subgroups for a scheme
    std::vector<PowerSubgroup> enumerateSubgroups(const Guid& schemeGuid,
                                                   std::error_code& ec) const;

    // --- Setting operations ---

    // Enumerate settings within a subgroup
    std::vector<PowerSetting> enumerateSettings(const Guid& schemeGuid,
                                                 const Guid& subgroupGuid,
                                                 std::error_code& ec) const;

    // Read friendly name for any power GUID (scheme / subgroup / setting)
    std::string readFriendlyName(const Guid& guid, std::error_code& ec) const;

    // Read value indices for a setting
    bool readACValueIndex(const Guid& schemeGuid, const Guid& subgroupGuid,
                          const Guid& settingGuid, uint32_t& valueIndex,
                          std::error_code& ec) const;

    bool readDCValueIndex(const Guid& schemeGuid, const Guid& subgroupGuid,
                          const Guid& settingGuid, uint32_t& valueIndex,
                          std::error_code& ec) const;

    // Write value indices for a setting
    bool writeACValueIndex(const Guid& schemeGuid, const Guid& subgroupGuid,
                           const Guid& settingGuid, uint32_t valueIndex,
                           std::error_code& ec) const;

    bool writeDCValueIndex(const Guid& schemeGuid, const Guid& subgroupGuid,
                           const Guid& settingGuid, uint32_t valueIndex,
                           std::error_code& ec) const;

    // Read possible value descriptions for a setting
    std::vector<SettingValue> readPossibleValues(const Guid& subgroupGuid,
                                                  const Guid& settingGuid,
                                                  std::error_code& ec) const;

    // --- GUID validation and info ---

    // Check if a GUID is a valid power scheme
    bool isScheme(const Guid& guid, std::error_code& ec) const;

    // Check if a GUID is a valid subgroup under a scheme
    bool isSubgroup(const Guid& schemeGuid, const Guid& subgroupGuid,
                    std::error_code& ec) const;

    // Check if a GUID is a valid setting under a scheme+subgroup
    bool isSetting(const Guid& schemeGuid, const Guid& subgroupGuid,
                   const Guid& settingGuid, std::error_code& ec) const;

    // Get detailed info for a setting GUID under a specific scheme+subgroup
    // Returns a PowerSetting with name, attributes, current AC/DC values
    // If the GUID doesn't exist, ec is set
    PowerSetting getSettingInfo(const Guid& schemeGuid, const Guid& subgroupGuid,
                                const Guid& settingGuid, std::error_code& ec) const;

    // --- Full scan ---

    // Build a complete tree: schemes -> subgroups -> settings (including hidden)
    std::vector<PowerScheme> fullScan(std::error_code& ec) const;

    // --- Hidden settings via registry ---

    // Enumerate hidden power settings from registry
    std::vector<PowerSetting> enumerateHiddenSettings(std::error_code& ec) const;
};

// Utility: convert Windows GUID to asul::Guid
Guid fromWindowsGuid(const GUID& g);

// Utility: convert asul::Guid to Windows GUID
GUID toWindowsGuid(const Guid& g);

} // namespace asul
