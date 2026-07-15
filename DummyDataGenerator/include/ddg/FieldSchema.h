#pragma once

#include "ddg/Json.h"

#include <string>

namespace ddg {

// Thrown when a field definition is missing a required attribute or holds
// an attribute of the wrong shape (e.g. "min" greater than "max").
class SchemaException : public std::runtime_error {
public:
    explicit SchemaException(const std::string& message) : std::runtime_error(message) {}
};

// Wraps a single field's raw JSON definition and exposes strongly-typed
// accessors for the attributes generators care about. FieldSchema does not
// hard-code a fixed set of types: any string in "type" is legal, and it is
// resolved against a GeneratorRegistry at generation time. This is what lets
// callers plug in project-specific field types without touching this class.
//
// Common attributes recognized by the built-in generators:
//   name            (string, required)   field name in the generated object
//   type            (string, required)   "int" | "double" | "bool" | "string" |
//                                         "enum" | "date" | "array" | "object" |
//                                         any custom type registered by the caller
//   nullable        (bool, default false) whether the field may generate null
//   nullProbability (number, default 0.3) probability of null when nullable
//   unique          (bool, default false) retry generation until the value is
//                                         unique within a single generation run
class FieldSchema {
public:
    explicit FieldSchema(json::JsonValue node);

    const std::string& GetName() const { return name_; }
    const std::string& GetType() const { return type_; }

    bool IsNullable() const;
    double GetNullProbability() const;
    bool IsUnique() const;

    bool Has(const std::string& key) const;

    long long GetInt(const std::string& key, long long defaultValue) const;
    double GetDouble(const std::string& key, double defaultValue) const;
    std::string GetString(const std::string& key, const std::string& defaultValue) const;
    bool GetBool(const std::string& key, bool defaultValue) const;

    // Throws SchemaException if the key is missing.
    const json::JsonValue& Require(const std::string& key) const;

    // Full access to the underlying JSON node, used by container generators
    // (array/object) that need to read nested field definitions.
    const json::JsonValue& Node() const { return node_; }

private:
    json::JsonValue node_;
    std::string name_;
    std::string type_;
};

} // namespace ddg
