# AsulPowerSettingsApi

**[English](README_EN.md) | 中文**

Windows 电源设置管理工具，复刻 [PowerSettingsExplorer](https://www.techpowerup.com/download/power-settings-explorer/) 的功能。通过 Windows Power API 和注册表，暴露被微软在控制面板中隐藏的电源设置。

## 功能特性

- 枚举所有电源方案、子组、设置及其 GUID
- 读写 AC/DC 电源设置值
- 发现被 Windows 隐藏的电源设置（190+ 项）
- 完整扫描模式：导出全部电源设置树形结构
- SDK 静态库，可集成到自己的项目中
- CLI 命令行工具，快速管理电源设置
- C++14，无外部依赖（纯 Windows API）

## 环境要求

- Windows 10/11
- CMake 3.14+
- C++14 兼容编译器：
  - MSVC 17+（Visual Studio 2022）
  - MinGW-w64（GCC 11+）

## 构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

构建产物：

| 产物           | 路径                                        |
| -------------- | ------------------------------------------- |
| SDK 静态库     | `build/lib/AsulPowerSettingsApi.lib`      |
| CLI 可执行文件 | `build/bin/power-settings.exe`            |
| 测试可执行文件 | `build/bin/AsulPowerSettingsApiTests.exe` |

### 构建选项

```bash
cmake .. -DBUILD_TESTS=OFF    # 禁用测试
cmake .. -DBUILD_CLI=OFF      # 禁用 CLI
```

## CLI 用法

```
power-settings <命令> [选项]
```

| 命令                                        | 说明                             |
| ------------------------------------------- | -------------------------------- |
| `list`                                    | 列出所有电源方案                 |
| `active`                                  | 显示当前活动电源方案             |
| `set-active <GUID>`                       | 切换活动电源方案                 |
| `subgroups <方案-GUID>`                   | 列出方案下的子组                 |
| `settings <方案-GUID> <子组-GUID>`        | 列出子组下的设置项               |
| `get <方案-GUID> <子组-GUID> <设置-GUID>` | 读取 AC/DC 值                    |
| `set ... --ac <值> --dc <值>`             | 写入 AC/DC 值                    |
| `hidden`                                  | 列出注册表中被隐藏的电源设置     |
| `scan`                                    | 完整扫描：枚举所有方案/子组/设置 |
| `create <源GUID> <名称>`                  | 基于现有方案创建新的电源方案     |
| `duplicate <GUID>`                        | 复制电源方案                     |
| `delete <GUID>`                           | 删除电源方案                     |
| `rename <GUID> <新名称>`                  | 重命名电源方案                   |
| `import <文件路径>`                       | 从 .pow 文件导入电源方案         |

### 使用示例

```bash
# 列出所有电源方案
power-settings list

# 显示当前活动方案
power-settings active

# 完整扫描所有设置
power-settings scan

# 列出被隐藏的设置
power-settings hidden

# 读取某个设置的值
power-settings get {381B4222-F694-41F0-9685-FF5BB260DF2E} {238C9FA8-0AAD-41ED-83F4-97BE242C8F20} {29F6C1DB-86DA-48C5-9FDB-F2B67B1F44DA}

# 设置"在此时间后睡眠"：接通电源永不，电池30分钟
power-settings set {381B4222-...} {238C9FA8-...} {29F6C1DB-...} --ac 0 --dc 1800
```

## SDK API

```cpp
#include <AsulPowerSettingsApi/PowerSettingsManager.h>

asul::PowerSettingsManager mgr;
std::error_code ec;

// 枚举所有方案
auto schemes = mgr.enumerateSchemes(ec);

// 获取当前活动方案
auto active = mgr.getActiveScheme(ec);

// 完整扫描（方案 -> 子组 -> 设置）
auto tree = mgr.fullScan(ec);

// 读写值
uint32_t val = 0;
mgr.readACValueIndex(schemeGuid, subgroupGuid, settingGuid, val, ec);
mgr.writeACValueIndex(schemeGuid, subgroupGuid, settingGuid, 1800, ec);

// 从注册表获取被隐藏的设置
auto hidden = mgr.enumerateHiddenSettings(ec);

// 创建新方案（基于"平衡"方案复制并重命名）
auto newGuid = mgr.createScheme(
    {0x381B4222, 0xF694, 0x41F0, {0x96,0x85,0xFF,0x5B,0xB2,0x60,0xDF,0x1E}},
    "我的自定义方案", ec);

// 复制方案
auto dupGuid = mgr.duplicateScheme(someGuid, ec);

// 重命名方案
mgr.renameScheme(someGuid, "新名称", ec);

// 删除方案
mgr.deleteScheme(someGuid, ec);

// 从 .pow 文件导入方案
auto importedGuid = mgr.importScheme("C:\\backup.pow", ec);
```

### 数据类型

| 类型                    | 说明                                      |
| ----------------------- | ----------------------------------------- |
| `asul::Guid`          | GUID 封装，支持`toString()`、比较运算符 |
| `asul::PowerScheme`   | 电源方案（如"平衡"），包含子组列表        |
| `asul::PowerSubgroup` | 子组（如"睡眠"），包含设置列表            |
| `asul::PowerSetting`  | 单个设置项，含 GUID、名称、属性、AC/DC 值 |
| `asul::SettingValue`  | 值描述（索引 + 名称）                     |

### 链接方式

```cmake
target_link_libraries(your_app PRIVATE AsulPowerSettingsApi)
```

## 测试

```bash
cd build
ctest
# 或直接运行：
./bin/AsulPowerSettingsApiTests.exe
```

14 个测试用例，覆盖 GUID 操作、方案枚举、子组/设置枚举、值读写、完整扫描、隐藏设置发现等功能。

## 项目结构

```
AsulPowerSettingsApi/
├── CMakeLists.txt
├── include/
│   ├── AsulPowerSettingsApi/
│   │   ├── Types.h                    # 数据结构定义
│   │   └── PowerSettingsManager.h     # SDK API 接口
│   └── cli/
│       └── CliApp.h
├── src/
│   ├── sdk/
│   │   ├── CMakeLists.txt
│   │   ├── PowerSettingsManager.cpp   # 核心实现
│   │   └── Types.cpp
│   └── cli/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── CliApp.cpp                 # CLI 命令实现
└── tests/
    ├── CMakeLists.txt
    ├── TestFramework.h
    ├── test_guid.cpp
    └── test_power_manager.cpp
```

## 技术原理

Windows 电源设置采用三级层次结构：

```
电源方案 (Power Scheme，如"平衡")
  └── 子组 (Subgroup，如"睡眠")
        └── 设置 (Setting，如"在此时间后睡眠")
```

- **枚举**：使用 `PowerEnumerate()` 配合 `ACCESS_SCHEME`、`ACCESS_SUBGROUP`、`ACCESS_INDIVIDUAL_SETTING` 访问标志
- **名称**：通过 `PowerReadFriendlyName()` 获取
- **值读写**：使用 `PowerReadACValueIndex()` / `PowerWriteACValueIndex()`（DC 变体同理）
- **隐藏设置**：读取注册表 `HKLM\SYSTEM\CurrentControlSet\Control\Power\PowerSettings` 下的 `Attributes` DWORD 值，第 0 位为 0 表示该设置被控制面板隐藏

## 许可证

MIT
