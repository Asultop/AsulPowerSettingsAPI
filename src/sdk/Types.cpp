#include <AsulPowerSettingsApi/Types.h>
#include <cstdio>
#include <cstring>
#include <cctype>

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

Guid Guid::fromString(const std::string& str, bool& ok) {
    ok = false;
    Guid g{0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

    // Expect format: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} (38 chars)
    // Also accept without braces: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX (36 chars)
    std::string s = str;

    // Strip braces if present
    if (!s.empty() && s.front() == '{') s.erase(0, 1);
    if (!s.empty() && s.back() == '}') s.pop_back();

    if (s.size() != 36) return g;
    if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') return g;

    // Helper: hex char to int, returns -1 on failure
    auto hexChar = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    // Helper: parse N hex chars into uint32_t
    auto parseHex = [&](size_t pos, int count, uint32_t& out) -> bool {
        out = 0;
        for (int i = 0; i < count; ++i) {
            int h = hexChar(s[pos + i]);
            if (h < 0) return false;
            out = (out << 4) | h;
        }
        return true;
    };

    uint32_t d1, d2, d3;
    if (!parseHex(0, 8, d1)) return g;
    if (!parseHex(9, 4, d2)) return g;
    if (!parseHex(14, 4, d3)) return g;

    uint8_t d4[8];
    for (int i = 0; i < 2; ++i) {
        uint32_t v;
        if (!parseHex(19 + i * 2, 2, v)) return g;
        d4[i] = static_cast<uint8_t>(v);
    }
    for (int i = 0; i < 6; ++i) {
        uint32_t v;
        if (!parseHex(24 + i * 2, 2, v)) return g;
        d4[2 + i] = static_cast<uint8_t>(v);
    }

    g.data1 = d1;
    g.data2 = static_cast<uint16_t>(d2);
    g.data3 = static_cast<uint16_t>(d3);
    std::memcpy(g.data4, d4, 8);
    ok = true;
    return g;
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
