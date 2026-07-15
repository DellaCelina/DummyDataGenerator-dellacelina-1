#include "ddg/RegexStringGenerator.h"

#include <cctype>
#include <string>
#include <vector>

namespace ddg {

struct RegexStringGenerator::Node {
    enum class Kind { CharSet, Concat, Alternation, Repeat };

    Kind kind;

    // CharSet
    std::vector<std::pair<unsigned char, unsigned char>> ranges;
    bool negate = false;

    // Concat / Alternation
    std::vector<std::unique_ptr<Node>> children;

    // Repeat
    std::unique_ptr<Node> child;
    int minRepeat = 0;
    int maxRepeat = 0;
};

namespace {

using Node = RegexStringGenerator::Node;
using Kind = Node::Kind;

std::unique_ptr<Node> MakeCharSet(std::vector<std::pair<unsigned char, unsigned char>> ranges, bool negate) {
    auto node = std::make_unique<Node>();
    node->kind = Kind::CharSet;
    node->ranges = std::move(ranges);
    node->negate = negate;
    return node;
}

std::unique_ptr<Node> MakeLiteral(char c) {
    return MakeCharSet({{static_cast<unsigned char>(c), static_cast<unsigned char>(c)}}, false);
}

char TranslateEscape(char e) {
    switch (e) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        default: return e;
    }
}

// Returns a shorthand class node for d/D/w/W/s/S, or nullptr if `e` is not a
// recognized shorthand (in which case the caller should treat it as a
// literal, escaped character).
std::unique_ptr<Node> TryMakeShorthandClass(char e) {
    switch (e) {
        case 'd': return MakeCharSet({{'0', '9'}}, false);
        case 'D': return MakeCharSet({{'0', '9'}}, true);
        case 'w': return MakeCharSet({{'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}}, false);
        case 'W': return MakeCharSet({{'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}}, true);
        case 's': return MakeCharSet({{' ', ' '}, {'\t', '\t'}}, false);
        case 'S': return MakeCharSet({{' ', ' '}, {'\t', '\t'}}, true);
        default: return nullptr;
    }
}

class RegexParser {
public:
    RegexParser(const std::string& pattern, int defaultMaxRepeat)
        : pattern_(pattern), defaultMaxRepeat_(defaultMaxRepeat) {}

    std::unique_ptr<Node> Parse() {
        auto result = ParseAlternation();
        if (!Eof()) {
            throw RegexParseException("Unmatched ')' in pattern: " + pattern_);
        }
        return result;
    }

private:
    const std::string& pattern_;
    size_t pos_ = 0;
    int defaultMaxRepeat_;

    bool Eof() const { return pos_ >= pattern_.size(); }
    char Peek() const { return pattern_[pos_]; }
    char Get() { return pattern_[pos_++]; }

    std::unique_ptr<Node> ParseAlternation() {
        auto first = ParseConcat();
        if (Eof() || Peek() != '|') return first;
        auto alt = std::make_unique<Node>();
        alt->kind = Kind::Alternation;
        alt->children.push_back(std::move(first));
        while (!Eof() && Peek() == '|') {
            Get();
            alt->children.push_back(ParseConcat());
        }
        return alt;
    }

    std::unique_ptr<Node> ParseConcat() {
        auto concat = std::make_unique<Node>();
        concat->kind = Kind::Concat;
        while (!Eof() && Peek() != '|' && Peek() != ')') {
            concat->children.push_back(ParseRepeat());
        }
        return concat;
    }

    std::unique_ptr<Node> ParseRepeat() {
        auto atom = ParseAtom();
        if (Eof()) return atom;

        int minRepeat = 1;
        int maxRepeat = 1;
        bool hasQuantifier = true;
        char c = Peek();
        if (c == '*') {
            Get();
            minRepeat = 0;
            maxRepeat = defaultMaxRepeat_;
        } else if (c == '+') {
            Get();
            minRepeat = 1;
            maxRepeat = defaultMaxRepeat_ < 1 ? 1 : defaultMaxRepeat_;
        } else if (c == '?') {
            Get();
            minRepeat = 0;
            maxRepeat = 1;
        } else if (c == '{') {
            ParseBraceQuantifier(minRepeat, maxRepeat);
        } else {
            hasQuantifier = false;
        }

        if (!hasQuantifier) return atom;
        if (minRepeat > maxRepeat) {
            throw RegexParseException("Invalid quantifier range in pattern: " + pattern_);
        }
        auto repeat = std::make_unique<Node>();
        repeat->kind = Kind::Repeat;
        repeat->child = std::move(atom);
        repeat->minRepeat = minRepeat;
        repeat->maxRepeat = maxRepeat;
        return repeat;
    }

    void ParseBraceQuantifier(int& minRepeat, int& maxRepeat) {
        Get(); // consume '{'
        std::string lowText;
        while (!Eof() && std::isdigit(static_cast<unsigned char>(Peek()))) lowText += Get();
        if (lowText.empty()) {
            throw RegexParseException("Invalid quantifier: expected a number after '{' in pattern: " + pattern_);
        }
        int low = std::stoi(lowText);
        int high = low;
        if (!Eof() && Peek() == ',') {
            Get();
            std::string highText;
            while (!Eof() && std::isdigit(static_cast<unsigned char>(Peek()))) highText += Get();
            if (highText.empty()) {
                high = low > defaultMaxRepeat_ ? low : defaultMaxRepeat_;
            } else {
                high = std::stoi(highText);
            }
        }
        if (Eof() || Peek() != '}') {
            throw RegexParseException("Invalid quantifier: expected '}' in pattern: " + pattern_);
        }
        Get(); // consume '}'
        minRepeat = low;
        maxRepeat = high;
    }

    std::unique_ptr<Node> ParseAtom() {
        if (Eof()) {
            throw RegexParseException("Unexpected end of pattern: " + pattern_);
        }
        char c = Peek();
        if (c == '(') {
            Get();
            if (!Eof() && Peek() == '?') {
                size_t save = pos_;
                Get();
                if (!Eof() && Peek() == ':') {
                    Get();
                } else {
                    pos_ = save;
                }
            }
            auto inner = ParseAlternation();
            if (Eof() || Peek() != ')') {
                throw RegexParseException("Unmatched '(' in pattern: " + pattern_);
            }
            Get();
            return inner;
        }
        if (c == '[') return ParseCharClass();
        if (c == '.') {
            Get();
            return MakeCharSet({{0x20, 0x7E}}, false);
        }
        if (c == '^' || c == '$') {
            Get();
            auto empty = std::make_unique<Node>();
            empty->kind = Kind::Concat;
            return empty;
        }
        if (c == '\\') {
            Get();
            if (Eof()) throw RegexParseException("Dangling escape in pattern: " + pattern_);
            char e = Get();
            if (auto shorthand = TryMakeShorthandClass(e)) return shorthand;
            return MakeLiteral(TranslateEscape(e));
        }
        Get();
        return MakeLiteral(c);
    }

    std::unique_ptr<Node> ParseCharClass() {
        Get(); // consume '['
        bool negate = false;
        if (!Eof() && Peek() == '^') {
            negate = true;
            Get();
        }
        std::vector<std::pair<unsigned char, unsigned char>> ranges;
        while (true) {
            if (Eof()) throw RegexParseException("Unmatched '[' in pattern: " + pattern_);
            if (Peek() == ']') {
                Get();
                break;
            }
            char lo;
            bool isShorthand = false;
            if (Peek() == '\\') {
                Get();
                if (Eof()) throw RegexParseException("Dangling escape in character class: " + pattern_);
                char e = Get();
                if (e == 'd' || e == 'w' || e == 's') {
                    auto shorthand = TryMakeShorthandClass(e);
                    ranges.insert(ranges.end(), shorthand->ranges.begin(), shorthand->ranges.end());
                    isShorthand = true;
                    lo = 0;
                } else if (e == 'D' || e == 'W' || e == 'S') {
                    throw RegexParseException("Negated shorthand classes are not supported inside a character class: " + pattern_);
                } else {
                    lo = TranslateEscape(e);
                }
            } else {
                lo = Get();
            }
            if (isShorthand) continue;

            if (!Eof() && Peek() == '-' && pos_ + 1 < pattern_.size() && pattern_[pos_ + 1] != ']') {
                Get(); // consume '-'
                char hi;
                if (Peek() == '\\') {
                    Get();
                    if (Eof()) throw RegexParseException("Dangling escape in character class: " + pattern_);
                    hi = TranslateEscape(Get());
                } else {
                    hi = Get();
                }
                ranges.push_back({static_cast<unsigned char>(lo), static_cast<unsigned char>(hi)});
            } else {
                ranges.push_back({static_cast<unsigned char>(lo), static_cast<unsigned char>(lo)});
            }
        }
        if (ranges.empty()) {
            throw RegexParseException("Empty character class is not allowed: " + pattern_);
        }
        return MakeCharSet(std::move(ranges), negate);
    }
};

std::string GenerateCharSet(const Node& node, RandomEngine& rng) {
    if (!node.negate) {
        long long total = 0;
        for (const auto& r : node.ranges) total += (r.second - r.first + 1);
        if (total <= 0) throw RegexParseException("Character class is empty");
        long long pick = rng.NextInt(0, total - 1);
        for (const auto& r : node.ranges) {
            long long size = r.second - r.first + 1;
            if (pick < size) return std::string(1, static_cast<char>(r.first + pick));
            pick -= size;
        }
        return std::string(1, static_cast<char>(node.ranges.back().second));
    }

    std::vector<unsigned char> allowed;
    for (int c = 0x20; c <= 0x7E; ++c) {
        bool excluded = false;
        for (const auto& r : node.ranges) {
            if (static_cast<unsigned char>(c) >= r.first && static_cast<unsigned char>(c) <= r.second) {
                excluded = true;
                break;
            }
        }
        if (!excluded) allowed.push_back(static_cast<unsigned char>(c));
    }
    if (allowed.empty()) throw RegexParseException("Negated character class excludes every printable character");
    return std::string(1, static_cast<char>(allowed[rng.NextIndex(allowed.size())]));
}

std::string GenerateNode(const Node& node, RandomEngine& rng) {
    switch (node.kind) {
        case Kind::CharSet:
            return GenerateCharSet(node, rng);
        case Kind::Concat: {
            std::string out;
            for (const auto& child : node.children) out += GenerateNode(*child, rng);
            return out;
        }
        case Kind::Alternation: {
            size_t idx = rng.NextIndex(node.children.size());
            return GenerateNode(*node.children[idx], rng);
        }
        case Kind::Repeat: {
            int count = node.minRepeat == node.maxRepeat
                            ? node.minRepeat
                            : static_cast<int>(rng.NextInt(node.minRepeat, node.maxRepeat));
            std::string out;
            for (int i = 0; i < count; ++i) out += GenerateNode(*node.child, rng);
            return out;
        }
    }
    return {};
}

} // namespace

RegexStringGenerator::RegexStringGenerator(const std::string& pattern, int defaultMaxRepeat)
    : defaultMaxRepeat_(defaultMaxRepeat) {
    if (defaultMaxRepeat < 0) {
        throw std::invalid_argument("RegexStringGenerator: defaultMaxRepeat must be >= 0");
    }
    RegexParser parser(pattern, defaultMaxRepeat);
    root_ = parser.Parse();
}

RegexStringGenerator::~RegexStringGenerator() = default;
RegexStringGenerator::RegexStringGenerator(RegexStringGenerator&&) noexcept = default;
RegexStringGenerator& RegexStringGenerator::operator=(RegexStringGenerator&&) noexcept = default;

std::string RegexStringGenerator::Generate(RandomEngine& rng) const {
    return GenerateNode(*root_, rng);
}

std::string RegexStringGenerator::GenerateFromPattern(const std::string& pattern, RandomEngine& rng, int defaultMaxRepeat) {
    RegexStringGenerator generator(pattern, defaultMaxRepeat);
    return generator.Generate(rng);
}

} // namespace ddg
