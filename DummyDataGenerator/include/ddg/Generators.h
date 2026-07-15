#pragma once

#include "ddg/IValueGenerator.h"

namespace ddg {

// type: "int" | "integer"
// attributes: min (default 0), max (default 100)
class IntGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "double" | "float" | "number"
// attributes: min (default 0.0), max (default 1.0), decimalPlaces (optional)
class DoubleGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "bool" | "boolean"
// attributes: trueProbability (default 0.5)
class BoolGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "string"
// attributes: pattern (regex subset, see RegexStringGenerator) OR
//             minLength/maxLength/length + charset (default alphanumeric)
//             regexMaxRepeat (default 6) caps unbounded regex quantifiers
class StringGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "enum"
// attributes: values (required, non-empty array of any JSON values)
class EnumGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "date"
// attributes: min, max (strings, default "1970-01-01"/"2038-01-01"),
//             format (strftime pattern, default "%Y-%m-%d")
class DateGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "array"
// attributes: items (required, a field-definition object describing each
//             element; "name" is optional and defaults to "item"),
//             minItems (default 0), maxItems (default minItems + 3)
class ArrayGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

// type: "object"
// attributes: fields (required, array of field-definition objects describing
//             a nested object, same shape as the top-level schema's "fields")
class ObjectGenerator : public IValueGenerator {
public:
    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override;
};

} // namespace ddg
