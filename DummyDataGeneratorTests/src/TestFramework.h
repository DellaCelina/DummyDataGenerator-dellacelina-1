#pragma once

// Minimal, dependency-free unit test framework. DummyDataGenerator is a
// PoC-scale C++ project with no package manager wired up, so rather than
// depend on GoogleTest/Catch2 being available on the grading machine, tests
// register themselves at static-init time and TestMain.cpp runs them all
// and reports a PASS/FAIL summary with a non-zero exit code on failure.

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ddg::test {

class AssertionFailure : public std::runtime_error {
public:
    explicit AssertionFailure(const std::string& message) : std::runtime_error(message) {}
};

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

std::vector<TestCase>& Registry();

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        Registry().push_back(TestCase{name, std::move(fn)});
    }
};

} // namespace ddg::test

#define DDG_CONCAT_INNER(a, b) a##b
#define DDG_CONCAT(a, b) DDG_CONCAT_INNER(a, b)

#define DDG_TEST(testName)                                                              \
    static void testName();                                                             \
    static ddg::test::Registrar DDG_CONCAT(registrar_, testName)(#testName, testName);  \
    static void testName()

#define DDG_CHECK(condition)                                                            \
    do {                                                                                \
        if (!(condition)) {                                                             \
            throw ddg::test::AssertionFailure(                                          \
                std::string("CHECK failed: ") + #condition + " (" + __FILE__ + ":" +    \
                std::to_string(__LINE__) + ")");                                        \
        }                                                                                \
    } while (false)

// NOTE: (actual) and (expected) are compared within a single full-expression
// on purpose. Splitting them into separate `auto&&` bindings across
// statements (as an earlier version of this macro did) is unsafe: many
// accessors here (JsonValue::AsString/At/AsArray/...) return a reference into
// an object that may itself be a temporary (e.g. JsonValue::Parse(...).AsString()).
// A reference bound to such a nested temporary is NOT lifetime-extended past
// the end of the statement that created it, so reading it from a later
// statement is undefined behavior. Keeping everything in one `if` condition
// keeps any such temporaries alive for the whole comparison.
#define DDG_CHECK_EQ(actual, expected)                                                  \
    do {                                                                                \
        if (!((actual) == (expected))) {                                                \
            throw ddg::test::AssertionFailure(                                          \
                std::string("CHECK_EQ failed: ") + #actual + " != " + #expected +       \
                " (" + __FILE__ + ":" + std::to_string(__LINE__) + ")");                \
        }                                                                                \
    } while (false)

#define DDG_CHECK_THROWS(expression)                                                    \
    do {                                                                                \
        bool ddg_threw_ = false;                                                        \
        try {                                                                           \
            (void)(expression);                                                         \
        } catch (...) {                                                                 \
            ddg_threw_ = true;                                                          \
        }                                                                               \
        if (!ddg_threw_) {                                                              \
            throw ddg::test::AssertionFailure(                                          \
                std::string("CHECK_THROWS failed: ") + #expression + " did not throw (" \
                + __FILE__ + ":" + std::to_string(__LINE__) + ")");                     \
        }                                                                                \
    } while (false)
