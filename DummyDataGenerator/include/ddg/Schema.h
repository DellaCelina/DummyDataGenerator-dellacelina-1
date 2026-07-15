#pragma once

#include "ddg/FieldSchema.h"
#include "ddg/Json.h"

#include <string>
#include <vector>

namespace ddg {

// A parsed schema describing one JSON object "shape" made of named,
// typed fields. This is the general-purpose structure definition the user
// hands to DataGenerator; it has no notion of "sample", "order", etc. and can
// describe any record shape a project needs.
//
// Expected JSON shape:
// {
//   "name": "OptionalSchemaName",
//   "fields": [
//     { "name": "id", "type": "string", "pattern": "S-\\d{4}", "unique": true },
//     { "name": "age", "type": "int", "min": 0, "max": 120 }
//   ]
// }
class Schema {
public:
    static Schema FromJson(const json::JsonValue& root);
    static Schema FromString(const std::string& jsonText);
    static Schema FromFile(const std::string& path);

    const std::string& GetName() const { return name_; }
    const std::vector<FieldSchema>& GetFields() const { return fields_; }

private:
    std::string name_;
    std::vector<FieldSchema> fields_;
};

} // namespace ddg
