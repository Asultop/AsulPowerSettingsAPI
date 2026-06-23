#include <AsulPowerSettingsApi/Types.h>
#include <cstdio>
#include <cstring>

namespace asul {

std::string Guid::toString() const {
    char buf[48];
    std::snprintf(buf, sizeof(buf),
        "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        data1, data2, data3,
        data4[0], data4[1], data4[2], data4[3],
        data4[4], data4[5], data4[6], data4[7]);
    return std::string(buf);
}

bool Guid::operator==(const Guid& rhs) const {
    return data1 == rhs.data1
        && data2 == rhs.data2
        && data3 == rhs.data3
        && std::memcmp(data4, rhs.data4, 8) == 0;
}

bool Guid::operator<(const Guid& rhs) const {
    if (data1 != rhs.data1) return data1 < rhs.data1;
    if (data2 != rhs.data2) return data2 < rhs.data2;
    if (data3 != rhs.data3) return data3 < rhs.data3;
    return std::memcmp(data4, rhs.data4, 8) < 0;
}

} // namespace asul
