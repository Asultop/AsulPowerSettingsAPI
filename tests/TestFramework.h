#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cstring>
#include <stdexcept>

// Minimal test framework
struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& getTests() {
    static std::vector<TestCase> t;
    return t;
}

inline int addTest(const char* name, std::function<void()> fn) {
    getTests().push_back({name, std::move(fn)});
    return 0;
}

#define TEST(name) \
    static void test_fn_##name(); \
    static int test_reg_##name = addTest(#name, test_fn_##name); \
    static void test_fn_##name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "  FAIL: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("Assertion failed: " #expr); \
        } \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::cerr << "  FAIL: " << #a << " == " << #b \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("Assertion failed"); \
        } \
    } while (0)

#define ASSERT_NE(a, b) ASSERT_TRUE(!((a) == (b)))

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (std::strcmp((a).c_str(), (b).c_str()) != 0) { \
            std::cerr << "  FAIL: string mismatch: \"" << (a) << "\" != \"" << (b) << "\"" \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("String assertion failed"); \
        } \
    } while (0)
