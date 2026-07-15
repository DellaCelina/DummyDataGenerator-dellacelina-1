#include "ddg/Schema.h"

#include <fstream>
#include <sstream>

namespace ddg {

Schema Schema::FromJson(const json::JsonValue& root) {
    if (!root.IsObject()) {
        throw SchemaException("Schema root must be a JSON object");
    }
    if (!root.Contains("fields") || !root.At("fields").IsArray()) {
        throw SchemaException("Schema is missing required array attribute 'fields'");
    }

    Schema schema;
    if (root.Contains("name") && root.At("name").IsString()) {
        schema.name_ = root.At("name").AsString();
    }

    const auto& fieldNodes = root.At("fields").AsArray();
    if (fieldNodes.empty()) {
        throw SchemaException("Schema 'fields' must contain at least one field definition");
    }
    schema.fields_.reserve(fieldNodes.size());
    for (const auto& node : fieldNodes) {
        schema.fields_.emplace_back(node);
    }
    return schema;
}

Schema Schema::FromString(const std::string& jsonText) {
    return FromJson(json::JsonValue::Parse(jsonText));
}

Schema Schema::FromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw SchemaException("Could not open schema file: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return FromString(buffer.str());
}

} // namespace ddg
