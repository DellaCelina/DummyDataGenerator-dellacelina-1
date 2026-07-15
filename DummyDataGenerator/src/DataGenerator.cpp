#include "ddg/DataGenerator.h"

#include <fstream>
#include <stdexcept>

namespace ddg {

DataGenerator::DataGenerator(Schema schema, uint64_t seed)
    : schema_(std::move(schema)), rng_(seed) {}

json::JsonValue DataGenerator::GenerateFieldForRecord(const FieldSchema& field) {
    if (!field.IsUnique()) {
        return GenerateFieldValue(field, rng_, registry_);
    }

    constexpr int kMaxAttempts = 100;
    auto& used = usedUniqueValues_[field.GetName()];
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        json::JsonValue value = GenerateFieldValue(field, rng_, registry_);
        std::string key = value.Dump(-1);
        if (used.insert(key).second) {
            return value;
        }
    }
    throw std::runtime_error(
        "DataGenerator: could not generate a unique value for field '" + field.GetName() +
        "' after " + std::to_string(kMaxAttempts) +
        " attempts. Widen its range/pattern or generate fewer unique records.");
}

json::JsonValue DataGenerator::GenerateRecord() {
    json::JsonValue::ObjectType obj;
    obj.reserve(schema_.GetFields().size());
    for (const auto& field : schema_.GetFields()) {
        obj.emplace_back(field.GetName(), GenerateFieldForRecord(field));
    }
    return json::JsonValue(std::move(obj));
}

json::JsonValue DataGenerator::GenerateOne() {
    return GenerateRecord();
}

json::JsonValue DataGenerator::GenerateMany(size_t count) {
    json::JsonValue::ArrayType records;
    records.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        records.push_back(GenerateRecord());
    }
    return json::JsonValue(std::move(records));
}

void DataGenerator::GenerateToFile(const std::string& path, size_t count, int indent) {
    json::JsonValue records = GenerateMany(count);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("DataGenerator: could not open output file: " + path);
    }
    file << records.Dump(indent);
}

} // namespace ddg
