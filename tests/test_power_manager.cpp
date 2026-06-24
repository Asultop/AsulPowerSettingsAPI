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

TEST(createAndDeleteScheme) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    // Get first scheme as source
    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!schemes.empty());

    // Create a new scheme by duplicating and renaming
    auto newGuid = mgr.createScheme(schemes[0].guid, "Test_Scheme_AsulPower", ec);
    ASSERT_FALSE(ec);

    // Verify the new scheme exists
    auto schemesAfter = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    bool found = false;
    for (const auto& s : schemesAfter) {
        if (s.guid == newGuid) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    // Rename the scheme
    bool renamed = mgr.renameScheme(newGuid, "Test_Scheme_Renamed", ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(renamed);

    // Verify renamed name
    std::string name = mgr.readFriendlyName(newGuid, ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(name == "Test_Scheme_Renamed");

    // Clean up: delete the test scheme
    bool deleted = mgr.deleteScheme(newGuid, ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(deleted);

    // Verify deleted
    auto schemesFinal = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    for (const auto& s : schemesFinal) {
        ASSERT_TRUE(s.guid != newGuid);
    }
}

TEST(duplicateScheme) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    auto schemes = mgr.enumerateSchemes(ec);
    ASSERT_FALSE(ec);
    ASSERT_TRUE(!schemes.empty());

    // Duplicate without renaming
    auto dupGuid = mgr.duplicateScheme(schemes[0].guid, ec);
    ASSERT_FALSE(ec);

    // Verify it exists
    auto name = mgr.readFriendlyName(dupGuid, ec);
    ASSERT_FALSE(ec);

    // Clean up
    mgr.deleteScheme(dupGuid, ec);
}

TEST(deleteScheme_invalidGuid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    // Try to delete a non-existent GUID
    asul::Guid fakeGuid{0xFFFFFFFF, 0xFFFF, 0xFFFF,
                        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    bool result = mgr.deleteScheme(fakeGuid, ec);
    ASSERT_FALSE(result);
    ASSERT_TRUE(ec);  // Should have an error
}

TEST(isScheme_valid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    // "Balanced" is always a valid scheme
    ASSERT_TRUE(mgr.isScheme(asul::PowerSchemeGuids::BALANCED, ec));
    ASSERT_FALSE(ec);
}

TEST(isScheme_invalid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    asul::Guid fakeGuid{0xFFFFFFFF, 0xFFFF, 0xFFFF,
                        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    ASSERT_FALSE(mgr.isScheme(fakeGuid, ec));
}

TEST(isSubgroup_valid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    // "Sleep" subgroup under "Balanced" should always exist
    ASSERT_TRUE(mgr.isSubgroup(asul::PowerSchemeGuids::BALANCED,
                               asul::PowerSubgroupGuids::SLEEP, ec));
    ASSERT_FALSE(ec);
}

TEST(isSubgroup_invalid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    asul::Guid fakeGuid{0xFFFFFFFF, 0xFFFF, 0xFFFF,
                        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    ASSERT_FALSE(mgr.isSubgroup(asul::PowerSchemeGuids::BALANCED, fakeGuid, ec));
}

TEST(isSetting_valid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    // "Sleep after" under "Sleep" under "Balanced" should always exist
    ASSERT_TRUE(mgr.isSetting(asul::PowerSchemeGuids::BALANCED,
                              asul::PowerSubgroupGuids::SLEEP,
                              asul::PowerSettingGuids::SLEEP_AFTER, ec));
    ASSERT_FALSE(ec);
}

TEST(isSetting_invalid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    asul::Guid fakeGuid{0xFFFFFFFF, 0xFFFF, 0xFFFF,
                        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    ASSERT_FALSE(mgr.isSetting(asul::PowerSchemeGuids::BALANCED,
                               asul::PowerSubgroupGuids::SLEEP,
                               fakeGuid, ec));
}

TEST(getSettingInfo_valid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    auto info = mgr.getSettingInfo(asul::PowerSchemeGuids::BALANCED,
                                   asul::PowerSubgroupGuids::SLEEP,
                                   asul::PowerSettingGuids::SLEEP_AFTER, ec);
    ASSERT_FALSE(ec);
    ASSERT_FALSE(info.name.empty());
    ASSERT_TRUE(info.guid == asul::PowerSettingGuids::SLEEP_AFTER);
}

TEST(getSettingInfo_invalid) {
    asul::PowerSettingsManager mgr;
    std::error_code ec;

    asul::Guid fakeGuid{0xFFFFFFFF, 0xFFFF, 0xFFFF,
                        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    auto info = mgr.getSettingInfo(asul::PowerSchemeGuids::BALANCED,
                                   asul::PowerSubgroupGuids::SLEEP,
                                   fakeGuid, ec);
    ASSERT_TRUE(ec);  // Should have an error
}
