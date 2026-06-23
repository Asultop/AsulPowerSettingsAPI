# AsulPowerSettingsApi

**English | [中文](README.md)**

A Windows power settings management tool that replicates [PowerSettingsExplorer](https://www.techpowerup.com/download/power-settings-explorer/) functionality. Exposes hidden power settings that Microsoft hides from the Control Panel, using the Windows Power API and registry.

## Features

- Enumerate all power schemes, subgroups, and settings with their GUIDs
- Read and write AC/DC power setting values
- Discover hidden power settings (190+ settings hidden by Windows)
- Full scan mode: dump the complete power settings tree
- SDK library for integration into your own projects
- CLI tool for quick command-line management
- C++14, no external dependencies (pure Windows API)

## Requirements

- Windows 10/11
- CMake 3.14+
- C++14 compatible compiler:
  - MSVC 17+ (Visual Studio 2022)
  - MinGW-w64 (GCC 11+)

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Build output:

| Artifact | Path |
|----------|------|
| SDK static library | `build/lib/AsulPowerSettingsApi.lib` |
| CLI executable | `build/bin/power-settings.exe` |
| Test executable | `build/bin/AsulPowerSettingsApiTests.exe` |

### Build Options

```bash
cmake .. -DBUILD_TESTS=OFF    # Disable tests
cmake .. -DBUILD_CLI=OFF      # Disable CLI
```

## CLI Usage

```
power-settings <command> [options]
```

| Command | Description |
|---------|-------------|
| `list` | List all power schemes |
| `active` | Show current active power scheme |
| `set-active <GUID>` | Switch active power scheme |
| `subgroups <scheme-GUID>` | List subgroups for a scheme |
| `settings <scheme-GUID> <subgroup-GUID>` | List settings in a subgroup |
| `get <scheme-GUID> <subgroup-GUID> <setting-GUID>` | Read AC/DC values |
| `set ... --ac <val> --dc <val>` | Write AC/DC values |
| `hidden` | List hidden power settings from registry |
| `scan` | Full scan: enumerate everything |
| `create <source-GUID> <name>` | Create a new power scheme from an existing one |
| `duplicate <GUID>` | Duplicate a power scheme |
| `delete <GUID>` | Delete a power scheme |
| `rename <GUID> <new-name>` | Rename a power scheme |
| `import <file-path>` | Import a power scheme from a .pow file |

### Examples

```bash
# List all power schemes
power-settings list

# Show active scheme
power-settings active

# Full scan of all settings
power-settings scan

# List hidden settings
power-settings hidden

# Get a setting value
power-settings get {381B4222-F694-41F0-9685-FF5BB260DF2E} {238C9FA8-0AAD-41ED-83F4-97BE242C8F20} {29F6C1DB-86DA-48C5-9FDB-F2B67B1F44DA}

# Set "Sleep after" to never on AC, 30 min on DC
power-settings set {381B4222-...} {238C9FA8-...} {29F6C1DB-...} --ac 0 --dc 1800
```

## SDK API

```cpp
#include <AsulPowerSettingsApi/PowerSettingsManager.h>

asul::PowerSettingsManager mgr;
std::error_code ec;

// Enumerate all schemes
auto schemes = mgr.enumerateSchemes(ec);

// Get active scheme
auto active = mgr.getActiveScheme(ec);

// Full scan (schemes -> subgroups -> settings)
auto tree = mgr.fullScan(ec);

// Read/write values
uint32_t val = 0;
mgr.readACValueIndex(schemeGuid, subgroupGuid, settingGuid, val, ec);
mgr.writeACValueIndex(schemeGuid, subgroupGuid, settingGuid, 1800, ec);

// Hidden settings from registry
auto hidden = mgr.enumerateHiddenSettings(ec);

// Create a new scheme (duplicate "Balanced" and rename)
auto newGuid = mgr.createScheme(
    {0x381B4222, 0xF694, 0x41F0, {0x96,0x85,0xFF,0x5B,0xB2,0x60,0xDF,0x1E}},
    "My Custom Scheme", ec);

// Duplicate a scheme
auto dupGuid = mgr.duplicateScheme(someGuid, ec);

// Rename a scheme
mgr.renameScheme(someGuid, "New Name", ec);

// Delete a scheme
mgr.deleteScheme(someGuid, ec);

// Import a scheme from a .pow file
auto importedGuid = mgr.importScheme("C:\\backup.pow", ec);
```

### Data Types

| Type | Description |
|------|-------------|
| `asul::Guid` | GUID wrapper with `toString()`, comparison operators |
| `asul::PowerScheme` | Power scheme (e.g. "Balanced") with subgroups |
| `asul::PowerSubgroup` | Subgroup (e.g. "Sleep") with settings |
| `asul::PowerSetting` | Individual setting with GUID, name, attributes, AC/DC values |
| `asul::SettingValue` | Value description (index + name) |

### Linking

```cmake
target_link_libraries(your_app PRIVATE AsulPowerSettingsApi)
```

## Tests

```bash
cd build
ctest
# or run directly:
./bin/AsulPowerSettingsApiTests.exe
```

14 tests covering GUID operations, scheme enumeration, subgroup/setting enumeration, value read/write, full scan, and hidden settings discovery.

## Project Structure

```
AsulPowerSettingsApi/
├── CMakeLists.txt
├── include/
│   ├── AsulPowerSettingsApi/
│   │   ├── Types.h                    # Data structures
│   │   └── PowerSettingsManager.h     # SDK API
│   └── cli/
│       └── CliApp.h
├── src/
│   ├── sdk/
│   │   ├── CMakeLists.txt
│   │   ├── PowerSettingsManager.cpp   # Core implementation
│   │   └── Types.cpp
│   └── cli/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── CliApp.cpp                 # CLI commands
└── tests/
    ├── CMakeLists.txt
    ├── TestFramework.h
    ├── test_guid.cpp
    └── test_power_manager.cpp
```

## How It Works

Windows power settings are organized in a three-level hierarchy:

```
Power Scheme (e.g. Balanced)
  └── Subgroup (e.g. Sleep)
        └── Setting (e.g. Sleep after)
```

- **Enumeration**: Uses `PowerEnumerate()` with `ACCESS_SCHEME`, `ACCESS_SUBGROUP`, and `ACCESS_INDIVIDUAL_SETTING` access flags
- **Names**: Retrieved via `PowerReadFriendlyName()`
- **Values**: Read/written with `PowerReadACValueIndex()` / `PowerWriteACValueIndex()` (and DC variants)
- **Hidden settings**: Discovered by reading the `Attributes` DWORD from `HKLM\SYSTEM\CurrentControlSet\Control\Power\PowerSettings`. Bit 0 = 0 means the setting is hidden from the Control Panel

## License

MIT
