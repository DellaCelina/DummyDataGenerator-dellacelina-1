#include "TestFramework.h"

#include <iostream>

int main() {
    const auto& tests = ddg::test::Registry();
    int passed = 0;
    int failed = 0;

    std::cout << "Running " << tests.size() << " test(s)...\n\n";
    for (const auto& test : tests) {
        try {
            test.fn();
            std::cout << "[PASS] " << test.name << "\n";
            ++passed;
        } catch (const std::exception& ex) {
            std::cout << "[FAIL] " << test.name << " - " << ex.what() << "\n";
            ++failed;
        } catch (...) {
            std::cout << "[FAIL] " << test.name << " - unknown exception\n";
            ++failed;
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed, " << tests.size() << " total\n";
    return failed == 0 ? 0 : 1;
}
