#pragma once

#include "ddg/FieldSchema.h"
#include "ddg/Json.h"
#include "ddg/RandomEngine.h"

#include <functional>

namespace ddg {

class GeneratorRegistry; // see GeneratorRegistry.h

// Extension point of the whole library: anything that can turn a
// FieldSchema into a JsonValue is a generator. Built-in generators cover
// int/double/bool/string/enum/date/array/object; callers register their own
// implementations under a new type name via GeneratorRegistry::Register to
// support project-specific field types (uuid, email, phone number, ...)
// without modifying this library.
class IValueGenerator {
public:
    virtual ~IValueGenerator() = default;

    // `registry` is passed through so container generators (array/object)
    // can resolve nested field types, and so custom generators can delegate
    // to existing ones if useful.
    virtual json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const = 0;
};

// Adapts a std::function into an IValueGenerator so callers can register a
// lambda instead of writing a full class.
class LambdaValueGenerator : public IValueGenerator {
public:
    using Func = std::function<json::JsonValue(const FieldSchema&, RandomEngine&, const GeneratorRegistry&)>;

    explicit LambdaValueGenerator(Func fn) : fn_(std::move(fn)) {}

    json::JsonValue Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const override {
        return fn_(field, rng, registry);
    }

private:
    Func fn_;
};

} // namespace ddg
