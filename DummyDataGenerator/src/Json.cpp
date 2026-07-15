#include "ddg/Json.h"

#include <sstream>
#include <cmath>
#include <cctype>
#include <iomanip>

namespace ddg::json {

namespace {

// Recursive-descent JSON parser. Operates on a single string and a cursor;
// throws JsonParseException with a character offset on any malformed input.
class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    JsonValue Parse() {
        SkipWhitespace();
        JsonValue result = ParseValue();
        SkipWhitespace();
        if (!Eof()) {
            Fail("Unexpected trailing characters after JSON value");
        }
        return result;
    }

private:
    const std::string& text_;
    size_t pos_ = 0;

    bool Eof() const { return pos_ >= text_.size(); }
    char Peek() const { return text_[pos_]; }
    char Get() { return text_[pos_++]; }

    [[noreturn]] void Fail(const std::string& message) const {
        throw JsonParseException(message + " (at offset " + std::to_string(pos_) + ")");
    }

    void SkipWhitespace() {
        while (!Eof()) {
            char c = Peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    void Expect(char c) {
        if (Eof() || Get() != c) {
            Fail(std::string("Expected '") + c + "'");
        }
    }

    bool Consume(const std::string& literal) {
        if (text_.compare(pos_, literal.size(), literal) == 0) {
            pos_ += literal.size();
            return true;
        }
        return false;
    }

    JsonValue ParseValue() {
        if (Eof()) Fail("Unexpected end of input");
        switch (Peek()) {
            case '{': return ParseObject();
            case '[': return ParseArray();
            case '"': return JsonValue(ParseString());
            case 't':
                if (Consume("true")) return JsonValue(true);
                Fail("Invalid literal");
            case 'f':
                if (Consume("false")) return JsonValue(false);
                Fail("Invalid literal");
            case 'n':
                if (Consume("null")) return JsonValue(nullptr);
                Fail("Invalid literal");
            default:
                return ParseNumber();
        }
    }

    JsonValue ParseObject() {
        Expect('{');
        JsonValue::ObjectType obj;
        SkipWhitespace();
        if (!Eof() && Peek() == '}') {
            Get();
            return JsonValue(std::move(obj));
        }
        while (true) {
            SkipWhitespace();
            if (Eof() || Peek() != '"') Fail("Expected string key");
            std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            SkipWhitespace();
            JsonValue value = ParseValue();
            obj.emplace_back(std::move(key), std::move(value));
            SkipWhitespace();
            if (Eof()) Fail("Unterminated object");
            char c = Get();
            if (c == ',') continue;
            if (c == '}') break;
            Fail("Expected ',' or '}'");
        }
        return JsonValue(std::move(obj));
    }

    JsonValue ParseArray() {
        Expect('[');
        JsonValue::ArrayType arr;
        SkipWhitespace();
        if (!Eof() && Peek() == ']') {
            Get();
            return JsonValue(std::move(arr));
        }
        while (true) {
            SkipWhitespace();
            arr.push_back(ParseValue());
            SkipWhitespace();
            if (Eof()) Fail("Unterminated array");
            char c = Get();
            if (c == ',') continue;
            if (c == ']') break;
            Fail("Expected ',' or ']'");
        }
        return JsonValue(std::move(arr));
    }

    std::string ParseString() {
        Expect('"');
        std::string out;
        while (true) {
            if (Eof()) Fail("Unterminated string");
            char c = Get();
            if (c == '"') break;
            if (c == '\\') {
                if (Eof()) Fail("Dangling escape in string");
                char e = Get();
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        unsigned int code = ParseHex4();
                        AppendUtf8(out, code);
                        break;
                    }
                    default:
                        Fail("Invalid escape sequence");
                }
            } else {
                out += c;
            }
        }
        return out;
    }

    unsigned int ParseHex4() {
        if (pos_ + 4 > text_.size()) Fail("Invalid \\u escape");
        unsigned int value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = Get();
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<unsigned int>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<unsigned int>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned int>(c - 'A' + 10);
            else Fail("Invalid hex digit in \\u escape");
        }
        return value;
    }

    static void AppendUtf8(std::string& out, unsigned int codepoint) {
        if (codepoint <= 0x7F) {
            out += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            out += static_cast<char>(0xC0 | (codepoint >> 6));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (codepoint >> 12));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    JsonValue ParseNumber() {
        size_t start = pos_;
        if (!Eof() && Peek() == '-') Get();
        if (Eof() || !std::isdigit(static_cast<unsigned char>(Peek()))) Fail("Invalid number");
        while (!Eof() && std::isdigit(static_cast<unsigned char>(Peek()))) Get();
        if (!Eof() && Peek() == '.') {
            Get();
            if (Eof() || !std::isdigit(static_cast<unsigned char>(Peek()))) Fail("Invalid number");
            while (!Eof() && std::isdigit(static_cast<unsigned char>(Peek()))) Get();
        }
        if (!Eof() && (Peek() == 'e' || Peek() == 'E')) {
            Get();
            if (!Eof() && (Peek() == '+' || Peek() == '-')) Get();
            if (Eof() || !std::isdigit(static_cast<unsigned char>(Peek()))) Fail("Invalid number");
            while (!Eof() && std::isdigit(static_cast<unsigned char>(Peek()))) Get();
        }
        std::string token = text_.substr(start, pos_ - start);
        try {
            return JsonValue(std::stod(token));
        } catch (const std::exception&) {
            Fail("Number out of range: " + token);
        }
    }
};

} // namespace

bool JsonValue::AsBool() const {
    if (auto p = std::get_if<bool>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not a bool");
}

double JsonValue::AsNumber() const {
    if (auto p = std::get_if<double>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not a number");
}

const std::string& JsonValue::AsString() const {
    if (auto p = std::get_if<std::string>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not a string");
}

const JsonValue::ArrayType& JsonValue::AsArray() const {
    if (auto p = std::get_if<ArrayType>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not an array");
}

JsonValue::ArrayType& JsonValue::AsArray() {
    if (auto p = std::get_if<ArrayType>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not an array");
}

const JsonValue::ObjectType& JsonValue::AsObject() const {
    if (auto p = std::get_if<ObjectType>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not an object");
}

JsonValue::ObjectType& JsonValue::AsObject() {
    if (auto p = std::get_if<ObjectType>(&value_)) return *p;
    throw JsonTypeException("JsonValue is not an object");
}

bool JsonValue::Contains(const std::string& key) const {
    if (!IsObject()) return false;
    for (const auto& [k, v] : AsObject()) {
        if (k == key) return true;
    }
    return false;
}

const JsonValue& JsonValue::At(const std::string& key) const {
    for (const auto& [k, v] : AsObject()) {
        if (k == key) return v;
    }
    throw JsonTypeException("Missing key '" + key + "' in JSON object");
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (IsNull()) {
        value_ = ObjectType{};
    }
    for (auto& [k, v] : AsObject()) {
        if (k == key) return v;
    }
    AsObject().emplace_back(key, JsonValue());
    return AsObject().back().second;
}

size_t JsonValue::Size() const {
    if (IsArray()) return AsArray().size();
    if (IsObject()) return AsObject().size();
    throw JsonTypeException("JsonValue has no Size()");
}

const JsonValue& JsonValue::At(size_t index) const {
    const auto& arr = AsArray();
    if (index >= arr.size()) throw JsonTypeException("Array index out of range");
    return arr[index];
}

JsonValue& JsonValue::operator[](size_t index) {
    auto& arr = AsArray();
    if (index >= arr.size()) throw JsonTypeException("Array index out of range");
    return arr[index];
}

void JsonValue::EscapeStringTo(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    out += oss.str();
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

namespace {
void DumpNumber(std::string& out, double d) {
    if (std::isfinite(d) && d == static_cast<double>(static_cast<long long>(d)) && std::fabs(d) < 1e15) {
        out += std::to_string(static_cast<long long>(d));
        return;
    }
    std::ostringstream oss;
    oss << std::setprecision(15) << d;
    out += oss.str();
}
} // namespace

void JsonValue::DumpTo(std::string& out, int indent, int depth) const {
    const bool pretty = indent >= 0;
    const std::string pad = pretty ? std::string(static_cast<size_t>(indent) * (depth + 1), ' ') : std::string();
    const std::string closePad = pretty ? std::string(static_cast<size_t>(indent) * depth, ' ') : std::string();
    const std::string newline = pretty ? "\n" : "";
    const std::string colon = pretty ? ": " : ":";

    switch (GetType()) {
        case Type::Null: out += "null"; break;
        case Type::Bool: out += AsBool() ? "true" : "false"; break;
        case Type::Number: DumpNumber(out, AsNumber()); break;
        case Type::String: EscapeStringTo(out, AsString()); break;
        case Type::Array: {
            const auto& arr = AsArray();
            if (arr.empty()) { out += "[]"; break; }
            out += '[';
            out += newline;
            for (size_t i = 0; i < arr.size(); ++i) {
                out += pad;
                arr[i].DumpTo(out, indent, depth + 1);
                if (i + 1 < arr.size()) out += ',';
                out += newline;
            }
            out += closePad;
            out += ']';
            break;
        }
        case Type::Object: {
            const auto& obj = AsObject();
            if (obj.empty()) { out += "{}"; break; }
            out += '{';
            out += newline;
            for (size_t i = 0; i < obj.size(); ++i) {
                out += pad;
                EscapeStringTo(out, obj[i].first);
                out += colon;
                obj[i].second.DumpTo(out, indent, depth + 1);
                if (i + 1 < obj.size()) out += ',';
                out += newline;
            }
            out += closePad;
            out += '}';
            break;
        }
    }
}

std::string JsonValue::Dump(int indent) const {
    std::string out;
    DumpTo(out, indent, 0);
    return out;
}

JsonValue JsonValue::Parse(const std::string& text) {
    Parser parser(text);
    return parser.Parse();
}

} // namespace ddg::json
