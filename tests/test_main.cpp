#include "TestFramework.h"

int main() {
    std::cout << "Running AsulPowerSettingsApi tests..." << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    int passed = 0, failed = 0, skipped = 0;
    for (const auto& tc : getTests()) {
        std::cout << "[TEST] " << tc.name << " ... ";
        try {
            tc.fn();
            std::cout << "PASS" << std::endl;
            ++passed;
        } catch (const std::exception& e) {
            if (std::strstr(e.what(), "SKIP") != nullptr) {
                std::cout << "SKIP" << std::endl;
                ++skipped;
            } else {
                std::cout << "FAIL (" << e.what() << ")" << std::endl;
                ++failed;
            }
        } catch (...) {
            std::cout << "FAIL (unknown exception)" << std::endl;
            ++failed;
        }
    }

    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Results: " << passed << " passed, "
              << failed << " failed, "
              << skipped << " skipped" << std::endl;
    return failed > 0 ? 1 : 0;
}
