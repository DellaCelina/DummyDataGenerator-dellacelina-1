#pragma once

#include "ddg/FieldSchema.h"
#include "ddg/IValueGenerator.h"
#include "ddg/Json.h"
#include "ddg/RandomEngine.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace ddg {

// Thrown when a field's "type" has no matching registered generator.
class UnknownFieldTypeException : public std::runtime_error {
public:
    explicit UnknownFieldTypeException(const std::string& message) : std::runtime_error(message) {}
};

// Maps a field type name (e.g. "int", "string", or a caller-defined type
// like "uuid") to the IValueGenerator that produces values for it. This is
// the mechanism that keeps DummyDataGenerator reusable across projects:
// a new project registers its own domain-specific generators instead of
// forking the library.
class GeneratorRegistry {
public:
    // Registers the built-in generators (int, double, bool, string, enum,
    // date, array, object).
    GeneratorRegistry();

    // Registers `generator` under `typeName`, overwriting any existing
    // generator registered for that name (including built-ins).
    void Register(const std::string& typeName, std::shared_ptr<IValueGenerator> generator);

    // Convenience overload for registering a lambda directly.
    void Register(const std::string& typeName, LambdaValueGenerator::Func fn);

    bool IsRegistered(const std::string& typeName) const;

    // Throws UnknownFieldTypeException if no generator is registered for
    // `typeName`.
    const IValueGenerator& Resolve(const std::string& typeName) const;

private:
    std::unordered_map<std::string, std::shared_ptr<IValueGenerator>> generators_;
};

// Generates the value for a single field, honoring "nullable" /
// "nullProbability" before delegating to the registered generator for the
// field's type. Container generators (array/object) call this for their
// nested fields so nullability behaves consistently everywhere.
json::JsonValue GenerateFieldValue(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry);

} // namespace ddg
