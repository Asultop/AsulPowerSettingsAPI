#include "TestFramework.h"
#include <AsulPowerSettingsApi/PowerSettingsManager.h>
#include <system_error>

TEST(enumerateSchemes) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(schemes.size() >= 1);
    for (const auto& s : schemes) {
        ASSERT_FALSE(s.name.empty());
    }
}

TEST(getActiveScheme) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto guid = mgr.getActiveScheme(ec);
    ASSERT_FALSE(ec);
    ASSERT_FALSE(guid.data1 == 0 && guid.data2 == 0 && guid.data3 == 0);
}

TEST(enumerateSubgroups) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!schemes.empty());

    auto subgroups = mgr.enumerateSubgroups(schemes[0].guid, ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(subgroups.size() >= 1);
}

TEST(enumerateSettings) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    auto subgroups = mgr.enumerateSubgroups(schemes[0].guid, ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!subgroups.empty());

    auto settings = mgr.enumerateSettings(schemes[0].guid, subgroups[0].guid, ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(settings.size() >= 1);
}

TEST(readACDCValues) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    auto subgroups = mgr.enumerateSubgroups(schemes[0].guid, ec);
    ASSERT_FALSE(ec);
    auto settings = mgr.enumerateSettings(schemes[0].guid, subgroups[0].guid, ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!settings.empty());

    uint32_t acVal = 0, dcVal = 0;
    mgr.readACValueIndex(schemes[0].guid, subgroups[0].guid,
                         settings[0].guid, acVal, ec);
    mgr.readDCValueIndex(schemes[0].guid, subgroups[0].guid,
                         settings[0].guid, dcVal, ec);
}

TEST(fullScan) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto result = mgr.fullScan(ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!result.empty());

    bool hasSubgroups = false;
    bool hasSettings = false;
    for (const auto& scheme : result) {
        if (!scheme.subgroups.empty()) hasSubgroups = true;
        for (const auto& sg : scheme.subgroups) {
            if (!sg.settings.empty()) hasSettings = true;
        }
    }
    ASSERT_TRUE(hasSubgroups);
    ASSERT_TRUE(hasSettings);
}

TEST(readFriendlyName) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!schemes.empty());

    std::string name = mgr.readFriendlyName(schemes[0].guid, ec);
    ASSERT_FALSE(ec);
    ASSERT_FALSE(name.empty());
}

TEST(guidConversion) {
    asul::Guid original{0x12345678, 0x9ABC, 0xDEF0,
                        {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    GUID win = asul::toWindowsGuid(original);
    asul::Guid back = asul::fromWindowsGuid(win);
    ASSERT_TRUE(original == back);
}

TEST(enumerateHiddenSettings) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;
    auto hidden = mgr.enumerateHiddenSettings(ec);
    if (ec) {
        // Registry access may require admin
        throw std::runtime_error("SKIP: registry access may require admin");
    }
}
