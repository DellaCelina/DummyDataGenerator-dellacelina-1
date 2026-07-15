#include "TestFramework.h"

namespace ddg::test {

std::vector<TestCase>& Registry() {
    static std::vector<TestCase> registry;
    return registry;
}

} // namespace ddg::test
