#include "ddg/FieldSchema.h"

#include <algorithm>
#include <cctype>

namespace ddg {

namespace {
std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
} // namespace

FieldSchema::FieldSchema(json::JsonValue node) : node_(std::move(node)) {
    if (!node_.IsObject()) {
        throw SchemaException("Field definition must be a JSON object");
    }
    if (!node_.Contains("name") || !node_.At("name").IsString()) {
        throw SchemaException("Field definition is missing required string attribute 'name'");
    }
    if (!node_.Contains("type") || !node_.At("type").IsString()) {
        throw SchemaException("Field '" + node_.At("name").AsString() + "' is missing required string attribute 'type'");
    }
    name_ = node_.At("name").AsString();
    type_ = ToLower(node_.At("type").AsString());
}

bool FieldSchema::IsNullable() const {
    return GetBool("nullable", false);
}

double FieldSchema::GetNullProbability() const {
    double p = GetDouble("nullProbability", 0.3);
    if (p < 0.0) p = 0.0;
    if (p > 1.0) p = 1.0;
    return p;
}

bool FieldSchema::IsUnique() const {
    return GetBool("unique", false);
}

bool FieldSchema::Has(const std::string& key) const {
    return node_.Contains(key);
}

long long FieldSchema::GetInt(const std::string& key, long long defaultValue) const {
    if (!Has(key)) return defaultValue;
    const auto& v = node_.At(key);
    if (!v.IsNumber()) {
        throw SchemaException("Field '" + name_ + "' attribute '" + key + "' must be a number");
    }
    return static_cast<long long>(v.AsNumber());
}

double FieldSchema::GetDouble(const std::string& key, double defaultValue) const {
    if (!Has(key)) return defaultValue;
    const auto& v = node_.At(key);
    if (!v.IsNumber()) {
        throw SchemaException("Field '" + name_ + "' attribute '" + key + "' must be a number");
    }
    return v.AsNumber();
}

std::string FieldSchema::GetString(const std::string& key, const std::string& defaultValue) const {
    if (!Has(key)) return defaultValue;
    const auto& v = node_.At(key);
    if (!v.IsString()) {
        throw SchemaException("Field '" + name_ + "' attribute '" + key + "' must be a string");
    }
    return v.AsString();
}

bool FieldSchema::GetBool(const std::string& key, bool defaultValue) const {
    if (!Has(key)) return defaultValue;
    const auto& v = node_.At(key);
    if (!v.IsBool()) {
        throw SchemaException("Field '" + name_ + "' attribute '" + key + "' must be a bool");
    }
    return v.AsBool();
}

const json::JsonValue& FieldSchema::Require(const std::string& key) const {
    if (!Has(key)) {
        throw SchemaException("Field '" + name_ + "' (type '" + type_ + "') is missing required attribute '" + key + "'");
    }
    return node_.At(key);
}

} // namespace ddg
