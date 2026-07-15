#pragma once

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <variant>
#include <cstdint>

namespace ddg::json {

// Thrown when JSON text cannot be parsed.
class JsonParseException : public std::runtime_error {
public:
    explicit JsonParseException(const std::string& message) : std::runtime_error(message) {}
};

// Thrown when a JsonValue is accessed as a type it does not hold.
class JsonTypeException : public std::runtime_error {
public:
    explicit JsonTypeException(const std::string& message) : std::runtime_error(message) {}
};

// A minimal, dependency-free JSON value used throughout DummyDataGenerator.
// Objects preserve insertion order so generated documents read the way the
// schema declared its fields.
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };
    using ArrayType = std::vector<JsonValue>;
    using ObjectType = std::vector<std::pair<std::string, JsonValue>>;

    JsonValue() : value_(std::monostate{}) {}
    JsonValue(std::nullptr_t) : value_(std::monostate{}) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(double d) : value_(d) {}
    JsonValue(int i) : value_(static_cast<double>(i)) {}
    JsonValue(long long i) : value_(static_cast<double>(i)) {}
    JsonValue(size_t i) : value_(static_cast<double>(i)) {}
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    JsonValue(const char* s) : value_(std::string(s)) {}
    JsonValue(ArrayType a) : value_(std::move(a)) {}
    JsonValue(ObjectType o) : value_(std::move(o)) {}

    Type GetType() const { return static_cast<Type>(value_.index()); }

    bool IsNull() const { return GetType() == Type::Null; }
    bool IsBool() const { return GetType() == Type::Bool; }
    bool IsNumber() const { return GetType() == Type::Number; }
    bool IsString() const { return GetType() == Type::String; }
    bool IsArray() const { return GetType() == Type::Array; }
    bool IsObject() const { return GetType() == Type::Object; }

    bool AsBool() const;
    double AsNumber() const;
    const std::string& AsString() const;
    const ArrayType& AsArray() const;
    ArrayType& AsArray();
    const ObjectType& AsObject() const;
    ObjectType& AsObject();

    // Object helpers. Contains()/operator[] only make sense for Type::Object.
    bool Contains(const std::string& key) const;
    const JsonValue& At(const std::string& key) const;      // throws JsonTypeException if missing
    JsonValue& operator[](const std::string& key);           // creates the key if missing
    const JsonValue& operator[](const std::string& key) const { return At(key); }

    // Array helpers.
    size_t Size() const;
    const JsonValue& At(size_t index) const;
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const { return At(index); }

    // Serializes the value. indent < 0 produces compact output, otherwise
    // pretty-prints using `indent` spaces per nesting level.
    std::string Dump(int indent = 2) const;

    static JsonValue Parse(const std::string& text);

    static JsonValue MakeArray() { return JsonValue(ArrayType{}); }
    static JsonValue MakeObject() { return JsonValue(ObjectType{}); }

private:
    std::variant<std::monostate, bool, double, std::string, ArrayType, ObjectType> value_;

    void DumpTo(std::string& out, int indent, int depth) const;
    static void EscapeStringTo(std::string& out, const std::string& s);
};

} // namespace ddg::json
