#include "TestFramework.h"
#include <AsulPowerSettingsApi/Types.h>

TEST(guid_toString) {
    asul::Guid g;
    g.data1 = 0x381B4222;
    g.data2 = 0xF694;
    g.data3 = 0x41F0;
    g.data4[0] = 0x96; g.data4[1] = 0x85;
    g.data4[2] = 0xFF; g.data4[3] = 0x5B;
    g.data4[4] = 0xB2; g.data4[5] = 0x60;
    g.data4[6] = 0xDF; g.data4[7] = 0x1E;

    std::string s = g.toString();
    ASSERT_TRUE(s.front() == '{');
    ASSERT_TRUE(s.back() == '}');
    ASSERT_TRUE(s.size() == 38);  // {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
}

TEST(guid_equality) {
    asul::Guid a{1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
    asul::Guid b{1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
    asul::Guid c{1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 12}};

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
    ASSERT_FALSE(a == c);
}

TEST(guid_ordering) {
    asul::Guid a{1, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
    asul::Guid b{2, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
    ASSERT_TRUE(a < b);
    ASSERT_FALSE(b < a);
}

TEST(setting_isHidden) {
    asul::PowerSetting ps;
    ps.attributes = 0;
    ASSERT_TRUE(ps.isHidden());
    ps.attributes = 1;
    ASSERT_FALSE(ps.isHidden());
    ps.attributes = 2;  // bit 0 still 0
    ASSERT_TRUE(ps.isHidden());
    ps.attributes = 3;  // bit 0 is 1
    ASSERT_FALSE(ps.isHidden());
}

TEST(subgroup_isHidden) {
    asul::PowerSubgroup sg;
    sg.attributes = 0;
    ASSERT_TRUE(sg.isHidden());
    sg.attributes = 1;
    ASSERT_FALSE(sg.isHidden());
}

TEST(guid_fromString_withBraces) {
    bool ok = false;
    auto g = asul::Guid::fromString("{381B4222-F694-41F0-9685-FF5BB260DF2E}", ok);
    ASSERT_TRUE(ok);
    ASSERT_EQ(g.data1, 0x381B4222u);
    ASSERT_EQ(g.data2, 0xF694);
    ASSERT_EQ(g.data3, 0x41F0);
    ASSERT_EQ(g.data4[0], 0x96);
    ASSERT_EQ(g.data4[1], 0x85);
    ASSERT_EQ(g.data4[2], 0xFF);
    ASSERT_EQ(g.data4[3], 0x5B);
    ASSERT_EQ(g.data4[4], 0xB2);
    ASSERT_EQ(g.data4[5], 0x60);
    ASSERT_EQ(g.data4[6], 0xDF);
    ASSERT_EQ(g.data4[7], 0x2E);
}

TEST(guid_fromString_withoutBraces) {
    bool ok = false;
    auto g = asul::Guid::fromString("381B4222-F694-41F0-9685-FF5BB260DF2E", ok);
    ASSERT_TRUE(ok);
    ASSERT_EQ(g.data1, 0x381B4222u);
}

TEST(guid_fromString_lowercase) {
    bool ok = false;
    auto g = asul::Guid::fromString("{381b4222-f694-41f0-9685-ff5bb260df2e}", ok);
    ASSERT_TRUE(ok);
    ASSERT_EQ(g.data1, 0x381B4222u);
    ASSERT_EQ(g.data4[2], 0xFF);
}

TEST(guid_fromString_invalid) {
    bool ok = false;
    asul::Guid::fromString("not-a-guid", ok);
    ASSERT_FALSE(ok);

    asul::Guid::fromString("{381B4222-F694-41F0}", ok);
    ASSERT_FALSE(ok);

    asul::Guid::fromString("", ok);
    ASSERT_FALSE(ok);
}

TEST(guid_fromString_roundTrip) {
    asul::Guid original{0x12345678, 0x9ABC, 0xDEF0,
                        {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    std::string str = original.toString();
    bool ok = false;
    auto parsed = asul::Guid::fromString(str, ok);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(parsed == original);
}
