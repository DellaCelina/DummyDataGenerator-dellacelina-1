#pragma once

#include "ddg/GeneratorRegistry.h"
#include "ddg/Json.h"
#include "ddg/RandomEngine.h"
#include "ddg/Schema.h"

#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ddg {

// Top-level entry point of the library: turns a Schema into any number of
// JSON records that follow it. This is the class most callers interact
// with directly.
//
// Example:
//   Schema schema = Schema::FromFile("sample_schema.json");
//   DataGenerator generator(schema);
//   generator.GetRegistry().Register("uuid", [](const FieldSchema&, RandomEngine& rng, const GeneratorRegistry&) {
//       return json::JsonValue(MakeRandomUuid(rng));
//   });
//   json::JsonValue records = generator.GenerateMany(100);
class DataGenerator {
public:
    explicit DataGenerator(Schema schema, uint64_t seed = std::random_device{}());

    // Exposes the registry so callers can register custom field types
    // before generating data.
    GeneratorRegistry& GetRegistry() { return registry_; }

    // Generates a single JSON object following the schema.
    json::JsonValue GenerateOne();

    // Generates a JSON array of `count` objects following the schema.
    json::JsonValue GenerateMany(size_t count);

    // Generates `count` records and writes them to `path` as a pretty
    // printed JSON array.
    void GenerateToFile(const std::string& path, size_t count, int indent = 2);

private:
    Schema schema_;
    RandomEngine rng_;
    GeneratorRegistry registry_;
    std::unordered_map<std::string, std::unordered_set<std::string>> usedUniqueValues_;

    json::JsonValue GenerateRecord();
    json::JsonValue GenerateFieldForRecord(const FieldSchema& field);
};

} // namespace ddg
