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
