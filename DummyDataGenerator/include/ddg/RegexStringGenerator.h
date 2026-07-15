#pragma once

#include "ddg/RandomEngine.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace ddg {

// Thrown when a "pattern" attribute cannot be parsed as a supported regex
// subset.
class RegexParseException : public std::runtime_error {
public:
    explicit RegexParseException(const std::string& message) : std::runtime_error(message) {}
};

// Generates random strings that match a (subset of) regular expression
// pattern. This is the inverse of std::regex: instead of testing whether a
// string matches, it produces strings that do.
//
// Supported syntax:
//   literals            a, 1, - (any character that is not special)
//   .                    any printable ASCII character
//   [abc] [a-z] [^a-z]  character classes, ranges, and negation
//   \d \D \w \W \s \S   shorthand classes (and their negations)
//   \n \t \r \\ \. etc. escaped characters
//   (...)  (?:...)      groups (both are treated as non-capturing)
//   a|b|c                alternation
//   * + ? {m} {m,} {m,n} quantifiers on the preceding atom
//   ^ $                  anchors (accepted but ignored; generation always
//                        produces a full match)
//
// Not supported: backreferences, lookaround, POSIX classes. Unbounded
// quantifiers (*, +, {m,}) are capped by `defaultMaxRepeat` (or the field's
// own repeat count for {m,n}) so generation always terminates.
class RegexStringGenerator {
public:
    explicit RegexStringGenerator(const std::string& pattern, int defaultMaxRepeat = 6);
    ~RegexStringGenerator();

    RegexStringGenerator(RegexStringGenerator&&) noexcept;
    RegexStringGenerator& operator=(RegexStringGenerator&&) noexcept;
    RegexStringGenerator(const RegexStringGenerator&) = delete;
    RegexStringGenerator& operator=(const RegexStringGenerator&) = delete;

    std::string Generate(RandomEngine& rng) const;

    // Convenience one-shot helper for callers that do not need to reuse a
    // parsed pattern across many calls.
    static std::string GenerateFromPattern(const std::string& pattern, RandomEngine& rng, int defaultMaxRepeat = 6);

    struct Node;

private:
    std::unique_ptr<Node> root_;
    int defaultMaxRepeat_;
};

} // namespace ddg
