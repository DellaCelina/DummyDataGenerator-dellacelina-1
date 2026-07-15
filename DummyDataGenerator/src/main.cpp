#include "ddg/DataGenerator.h"
#include "ddg/GeneratorRegistry.h"
#include "ddg/Json.h"
#include "ddg/Schema.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void PrintUsage(const char* argv0) {
    std::cout
        << "DummyDataGenerator - schema-driven JSON dummy data generator\n\n"
        << "Usage:\n"
        << "  " << argv0 << "                                  run the built-in demo\n"
        << "  " << argv0 << " <schema.json> [count] [output.json] [seed]\n\n"
        << "  schema.json   path to a JSON schema (see docs/schema-samples)\n"
        << "  count         number of records to generate (default: 5)\n"
        << "  output.json   file to write; if omitted, prints to stdout\n"
        << "  seed          integer seed for reproducible output (default: random)\n";
}

// Registers a small, deliberately non-regex-expressible custom generator to
// demonstrate how a project extends DummyDataGenerator with domain-specific
// field types without touching the library itself.
void RegisterDemoExtensions(ddg::GeneratorRegistry& registry) {
    static const std::array<std::string, 5> kSurnames = {"Kim", "Lee", "Park", "Choi", "Jung"};
    static const std::array<std::string, 6> kGivenNames = {"Minjun", "Seoyeon", "Jiho", "Hana", "Doyoon", "Yerin"};

    registry.Register("koreanName", [](const ddg::FieldSchema&, ddg::RandomEngine& rng, const ddg::GeneratorRegistry&) {
        const std::string& surname = kSurnames[rng.NextIndex(kSurnames.size())];
        const std::string& given = kGivenNames[rng.NextIndex(kGivenNames.size())];
        return ddg::json::JsonValue(surname + " " + given);
    });
}

// The schema from the "반도체 시료 생산주문관리 시스템" spec: a sample (시료)
// catalog entry, matching the "시료 등록" fields (ID, name, average
// production time, yield) plus a few extras that show off nullable,
// nested object, and array support.
std::string EmbeddedDemoSchema() {
    return R"JSON({
  "name": "SemiconductorSample",
  "fields": [
    { "name": "sampleId", "type": "string", "pattern": "S-\\d{3}", "unique": true },
    { "name": "sampleName", "type": "string", "pattern": "(Silicon Wafer|GaN Epitaxial|SiC Power|Photoresist)-[0-9]{1,2}(inch|mm)" },
    { "name": "avgProductionTimeMin", "type": "double", "min": 0.1, "max": 2.0, "decimalPlaces": 2 },
    { "name": "yield", "type": "double", "min": 0.7, "max": 0.99, "decimalPlaces": 2 },
    { "name": "stock", "type": "int", "min": 0, "max": 1000 },
    { "name": "status", "type": "enum", "values": ["RESERVED", "CONFIRMED", "PRODUCING", "RELEASE"] },
    { "name": "registeredAt", "type": "date", "min": "2025-01-01", "max": "2026-12-31" },
    { "name": "assignedEngineer", "type": "koreanName" },
    { "name": "note", "type": "string", "pattern": "[a-z]{5,10}", "nullable": true, "nullProbability": 0.3 },
    {
      "name": "tags", "type": "array", "minItems": 1, "maxItems": 3,
      "items": { "type": "string", "pattern": "[a-z]{3,6}" }
    },
    {
      "name": "customer", "type": "object",
      "fields": [
        { "name": "company", "type": "string", "pattern": "[A-Z][a-z]{3,7} (Labs|Fab|Research)" },
        { "name": "vip", "type": "bool", "trueProbability": 0.2 }
      ]
    }
  ]
})JSON";
}

int RunDemo() {
    std::cout << "== DummyDataGenerator demo (반도체 시료 생산주문관리 예시 스키마) ==\n\n";
    ddg::Schema schema = ddg::Schema::FromString(EmbeddedDemoSchema());
    ddg::DataGenerator generator(schema, /*seed=*/42);
    RegisterDemoExtensions(generator.GetRegistry());

    ddg::json::JsonValue records = generator.GenerateMany(5);
    std::cout << records.Dump(2) << "\n";
    return 0;
}

int RunFromArgs(int argc, char** argv) {
    std::string schemaPath = argv[1];
    size_t count = 5;
    std::string outputPath;
    bool haveSeed = false;
    uint64_t seed = 0;

    if (argc >= 3) {
        try {
            count = static_cast<size_t>(std::stoull(argv[2]));
        } catch (const std::exception&) {
            std::cerr << "Invalid count: " << argv[2] << "\n";
            return 1;
        }
    }
    if (argc >= 4) {
        outputPath = argv[3];
    }
    if (argc >= 5) {
        try {
            seed = static_cast<uint64_t>(std::stoull(argv[4]));
            haveSeed = true;
        } catch (const std::exception&) {
            std::cerr << "Invalid seed: " << argv[4] << "\n";
            return 1;
        }
    }

    try {
        ddg::Schema schema = ddg::Schema::FromFile(schemaPath);
        ddg::DataGenerator generator = haveSeed ? ddg::DataGenerator(schema, seed) : ddg::DataGenerator(schema);

        if (outputPath.empty()) {
            ddg::json::JsonValue records = generator.GenerateMany(count);
            std::cout << records.Dump(2) << "\n";
        } else {
            generator.GenerateToFile(outputPath, count);
            std::cout << "Wrote " << count << " record(s) to " << outputPath << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        PrintUsage(argv[0]);
        return 0;
    }
    if (argc == 1) {
        return RunDemo();
    }
    return RunFromArgs(argc, argv);
}
