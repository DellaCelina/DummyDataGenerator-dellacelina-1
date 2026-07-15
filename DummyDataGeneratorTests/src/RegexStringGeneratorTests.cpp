#include "TestFramework.h"
#include "ddg/RegexStringGenerator.h"

#include <regex>
#include <string>

using ddg::RandomEngine;
using ddg::RegexParseException;
using ddg::RegexStringGenerator;

namespace {

// Generates many strings from `pattern` and cross-checks every one of them
// against std::regex_match on the *same* pattern text. Since our engine only
// ever produces strings its own parser accepted, and std::regex implements
// the (much larger) ECMAScript grammar that is a superset of what we
// support, a full match here is strong evidence the generator is correct.
void CheckPatternMatchesItself(const std::string& pattern, int iterations = 100) {
    RegexStringGenerator generator(pattern);
    RandomEngine rng(12345);
    std::regex re(pattern);
    for (int i = 0; i < iterations; ++i) {
        std::string s = generator.Generate(rng);
        if (!std::regex_match(s, re)) {
            throw ddg::test::AssertionFailure(
                "Generated string '" + s + "' does not match pattern '" + pattern + "'");
        }
    }
}

} // namespace

DDG_TEST(Regex_LiteralAndDigitShorthand) {
    CheckPatternMatchesItself("S-\\d{3}");
}

DDG_TEST(Regex_CharacterClassWithRangeAndQuantifier) {
    CheckPatternMatchesItself("[A-Z]{2}\\d{4}");
}

DDG_TEST(Regex_Alternation) {
    CheckPatternMatchesItself("(cat|dog|bird)");
}

DDG_TEST(Regex_BoundedQuantifierRange) {
    CheckPatternMatchesItself("[a-z]{3,6}");
}

DDG_TEST(Regex_OptionalAndStarAndPlus) {
    CheckPatternMatchesItself("a?b+c*");
}

DDG_TEST(Regex_NegatedCharacterClass) {
    CheckPatternMatchesItself("[^0-9]{5}");
}

DDG_TEST(Regex_EmailLikePattern) {
    CheckPatternMatchesItself("\\w+@\\w+\\.(com|net)");
}

DDG_TEST(Regex_NonCapturingGroup) {
    CheckPatternMatchesItself("(?:ab|cd){2}");
}

DDG_TEST(Regex_UuidLikePattern) {
    CheckPatternMatchesItself("[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}");
}

DDG_TEST(Regex_SameSeedProducesSameSequence) {
    RegexStringGenerator generator("[A-Za-z0-9]{8}");
    RandomEngine rngA(777);
    RandomEngine rngB(777);
    for (int i = 0; i < 20; ++i) {
        DDG_CHECK_EQ(generator.Generate(rngA), generator.Generate(rngB));
    }
}

DDG_TEST(Regex_UnboundedQuantifierIsCapped) {
    RegexStringGenerator generator("a*", /*defaultMaxRepeat=*/4);
    RandomEngine rng(1);
    for (int i = 0; i < 50; ++i) {
        DDG_CHECK(generator.Generate(rng).size() <= 4);
    }
}

DDG_TEST(Regex_UnmatchedParenThrows) {
    DDG_CHECK_THROWS(RegexStringGenerator("(ab"));
    DDG_CHECK_THROWS(RegexStringGenerator("ab)"));
}

DDG_TEST(Regex_InvalidQuantifierThrows) {
    DDG_CHECK_THROWS(RegexStringGenerator("a{3,1}"));
    DDG_CHECK_THROWS(RegexStringGenerator("a{"));
}

DDG_TEST(Regex_EmptyCharacterClassThrows) {
    DDG_CHECK_THROWS(RegexStringGenerator("[]"));
}
