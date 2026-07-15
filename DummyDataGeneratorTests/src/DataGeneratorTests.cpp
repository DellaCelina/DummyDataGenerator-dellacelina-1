#include "TestFramework.h"
#include "ddg/DataGenerator.h"

#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

using ddg::DataGenerator;
using ddg::FieldSchema;
using ddg::GeneratorRegistry;
using ddg::RandomEngine;
using ddg::Schema;
using ddg::json::JsonValue;

namespace {
const char* kSampleSchema = R"({
    "name": "SemiconductorSample",
    "fields": [
        { "name": "sampleId", "type": "string", "pattern": "S-\\d{4}", "unique": true },
        { "name": "avgProductionTimeMin", "type": "double", "min": 0.1, "max": 2.0, "decimalPlaces": 2 },
        { "name": "yield", "type": "double", "min": 0.7, "max": 0.99, "decimalPlaces": 2 },
        { "name": "stock", "type": "int", "min": 0, "max": 1000 },
        { "name": "status", "type": "enum", "values": ["RESERVED", "CONFIRMED", "PRODUCING", "RELEASE"] }
    ]
})";
} // namespace

DDG_TEST(DataGenerator_GenerateManyProducesRequestedCount) {
    Schema schema = Schema::FromString(kSampleSchema);
    DataGenerator generator(schema, 100);
    JsonValue records = generator.GenerateMany(25);
    DDG_CHECK_EQ(records.Size(), static_cast<size_t>(25));
    for (size_t i = 0; i < records.Size(); ++i) {
        DDG_CHECK(records.At(i).Contains("sampleId"));
        DDG_CHECK(records.At(i).Contains("status"));
    }
}

DDG_TEST(DataGenerator_SameSeedIsReproducible) {
    Schema schema = Schema::FromString(kSampleSchema);
    DataGenerator generatorA(schema, 555);
    DataGenerator generatorB(schema, 555);
    DDG_CHECK_EQ(generatorA.GenerateMany(10).Dump(-1), generatorB.GenerateMany(10).Dump(-1));
}

DDG_TEST(DataGenerator_DifferentSeedsUsuallyDiffer) {
    Schema schema = Schema::FromString(kSampleSchema);
    DataGenerator generatorA(schema, 1);
    DataGenerator generatorB(schema, 2);
    DDG_CHECK(generatorA.GenerateMany(10).Dump(-1) != generatorB.GenerateMany(10).Dump(-1));
}

DDG_TEST(DataGenerator_UniqueFieldNeverRepeatsWithinARun) {
    Schema schema = Schema::FromString(R"({
        "fields": [
            { "name": "id", "type": "int", "min": 0, "max": 9, "unique": true }
        ]
    })");
    DataGenerator generator(schema, 42);
    JsonValue records = generator.GenerateMany(10);
    std::set<long long> seen;
    for (size_t i = 0; i < records.Size(); ++i) {
        long long id = static_cast<long long>(records.At(i).At("id").AsNumber());
        DDG_CHECK(seen.insert(id).second);
    }
}

DDG_TEST(DataGenerator_UniqueFieldThrowsWhenSpaceExhausted) {
    Schema schema = Schema::FromString(R"({
        "fields": [
            { "name": "id", "type": "int", "min": 0, "max": 2, "unique": true }
        ]
    })");
    DataGenerator generator(schema, 42);
    DDG_CHECK_THROWS(generator.GenerateMany(10));
}

DDG_TEST(DataGenerator_GenerateToFileWritesReadableJson) {
    Schema schema = Schema::FromString(kSampleSchema);
    DataGenerator generator(schema, 9);
    std::string path = "ddg_test_output.json";
    generator.GenerateToFile(path, 3);

    std::ifstream file(path, std::ios::binary);
    DDG_CHECK(static_cast<bool>(file));
    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    JsonValue parsed = JsonValue::Parse(buffer.str());
    DDG_CHECK_EQ(parsed.Size(), static_cast<size_t>(3));
    std::remove(path.c_str());
}

DDG_TEST(DataGenerator_CustomRegisteredGeneratorIsUsedEndToEnd) {
    Schema schema = Schema::FromString(R"({
        "fields": [ { "name": "greeting", "type": "shout" } ]
    })");
    DataGenerator generator(schema, 1);
    generator.GetRegistry().Register("shout", [](const FieldSchema&, RandomEngine&, const GeneratorRegistry&) {
        return JsonValue(std::string("HELLO"));
    });
    JsonValue records = generator.GenerateMany(3);
    for (size_t i = 0; i < records.Size(); ++i) {
        DDG_CHECK_EQ(records.At(i).At("greeting").AsString(), std::string("HELLO"));
    }
}

DDG_TEST(DataGenerator_NestedArrayAndObjectFieldsGenerate) {
    Schema schema = Schema::FromString(R"({
        "fields": [
            { "name": "tags", "type": "array", "minItems": 2, "maxItems": 2, "items": { "type": "string", "pattern": "[a-z]{3}" } },
            { "name": "customer", "type": "object", "fields": [
                { "name": "vip", "type": "bool", "trueProbability": 1.0 }
            ]}
        ]
    })");
    DataGenerator generator(schema, 3);
    JsonValue record = generator.GenerateOne();
    DDG_CHECK_EQ(record.At("tags").Size(), static_cast<size_t>(2));
    DDG_CHECK(record.At("customer").At("vip").AsBool());
}
