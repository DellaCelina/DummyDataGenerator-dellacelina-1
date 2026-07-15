#include "TestFramework.h"
#include "ddg/GeneratorRegistry.h"

#include <cmath>

using ddg::FieldSchema;
using ddg::GenerateFieldValue;
using ddg::GeneratorRegistry;
using ddg::RandomEngine;
using ddg::json::JsonValue;

namespace {
FieldSchema MakeField(const std::string& jsonText) {
    return FieldSchema(JsonValue::Parse(jsonText));
}
} // namespace

DDG_TEST(IntGenerator_StaysWithinRange) {
    GeneratorRegistry registry;
    RandomEngine rng(1);
    FieldSchema field = MakeField(R"({"name":"n","type":"int","min":10,"max":15})");
    for (int i = 0; i < 200; ++i) {
        double v = registry.Resolve("int").Generate(field, rng, registry).AsNumber();
        DDG_CHECK(v >= 10.0 && v <= 15.0);
    }
}

DDG_TEST(IntGenerator_RejectsInvertedRange) {
    GeneratorRegistry registry;
    RandomEngine rng(1);
    FieldSchema field = MakeField(R"({"name":"n","type":"int","min":10,"max":5})");
    DDG_CHECK_THROWS(registry.Resolve("int").Generate(field, rng, registry));
}

DDG_TEST(DoubleGenerator_RoundsToDecimalPlaces) {
    GeneratorRegistry registry;
    RandomEngine rng(2);
    FieldSchema field = MakeField(R"({"name":"y","type":"double","min":0.0,"max":1.0,"decimalPlaces":2})");
    for (int i = 0; i < 50; ++i) {
        double v = registry.Resolve("double").Generate(field, rng, registry).AsNumber();
        double scaled = v * 100.0;
        DDG_CHECK(std::fabs(scaled - std::round(scaled)) < 1e-6);
    }
}

DDG_TEST(BoolGenerator_HonorsExtremeProbabilities) {
    GeneratorRegistry registry;
    RandomEngine rng(3);
    FieldSchema alwaysTrue = MakeField(R"({"name":"b","type":"bool","trueProbability":1.0})");
    FieldSchema alwaysFalse = MakeField(R"({"name":"b","type":"bool","trueProbability":0.0})");
    for (int i = 0; i < 20; ++i) {
        DDG_CHECK(registry.Resolve("bool").Generate(alwaysTrue, rng, registry).AsBool() == true);
        DDG_CHECK(registry.Resolve("bool").Generate(alwaysFalse, rng, registry).AsBool() == false);
    }
}

DDG_TEST(StringGenerator_UsesPatternWhenPresent) {
    GeneratorRegistry registry;
    RandomEngine rng(4);
    FieldSchema field = MakeField(R"({"name":"s","type":"string","pattern":"[a-z]{4}"})");
    for (int i = 0; i < 20; ++i) {
        std::string s = registry.Resolve("string").Generate(field, rng, registry).AsString();
        DDG_CHECK_EQ(s.size(), static_cast<size_t>(4));
        for (char c : s) DDG_CHECK(c >= 'a' && c <= 'z');
    }
}

DDG_TEST(StringGenerator_UsesLengthBoundsWithoutPattern) {
    GeneratorRegistry registry;
    RandomEngine rng(5);
    FieldSchema field = MakeField(R"({"name":"s","type":"string","minLength":3,"maxLength":6})");
    for (int i = 0; i < 50; ++i) {
        std::string s = registry.Resolve("string").Generate(field, rng, registry).AsString();
        DDG_CHECK(s.size() >= 3 && s.size() <= 6);
    }
}

DDG_TEST(EnumGenerator_OnlyReturnsListedValues) {
    GeneratorRegistry registry;
    RandomEngine rng(6);
    FieldSchema field = MakeField(R"({"name":"status","type":"enum","values":["RESERVED","CONFIRMED","RELEASE"]})");
    for (int i = 0; i < 50; ++i) {
        std::string v = registry.Resolve("enum").Generate(field, rng, registry).AsString();
        DDG_CHECK(v == "RESERVED" || v == "CONFIRMED" || v == "RELEASE");
    }
}

DDG_TEST(EnumGenerator_RequiresNonEmptyValues) {
    GeneratorRegistry registry;
    RandomEngine rng(6);
    FieldSchema missingValues = MakeField(R"({"name":"status","type":"enum"})");
    FieldSchema emptyValues = MakeField(R"({"name":"status","type":"enum","values":[]})");
    DDG_CHECK_THROWS(registry.Resolve("enum").Generate(missingValues, rng, registry));
    DDG_CHECK_THROWS(registry.Resolve("enum").Generate(emptyValues, rng, registry));
}

DDG_TEST(DateGenerator_StaysWithinRangeAndFormats) {
    GeneratorRegistry registry;
    RandomEngine rng(7);
    FieldSchema field = MakeField(R"({"name":"d","type":"date","min":"2025-01-01","max":"2025-01-03"})");
    bool sawJan1 = false, sawJan2 = false, sawJan3 = false;
    for (int i = 0; i < 200; ++i) {
        std::string d = registry.Resolve("date").Generate(field, rng, registry).AsString();
        DDG_CHECK_EQ(d.size(), static_cast<size_t>(10));
        DDG_CHECK(d == "2025-01-01" || d == "2025-01-02" || d == "2025-01-03");
        sawJan1 |= (d == "2025-01-01");
        sawJan2 |= (d == "2025-01-02");
        sawJan3 |= (d == "2025-01-03");
    }
    DDG_CHECK(sawJan1 && sawJan2 && sawJan3);
}

DDG_TEST(DateGenerator_RejectsMalformedDate) {
    GeneratorRegistry registry;
    RandomEngine rng(7);
    FieldSchema field = MakeField(R"({"name":"d","type":"date","min":"not-a-date"})");
    DDG_CHECK_THROWS(registry.Resolve("date").Generate(field, rng, registry));
}

DDG_TEST(ArrayGenerator_RespectsItemCountBounds) {
    GeneratorRegistry registry;
    RandomEngine rng(8);
    FieldSchema field = MakeField(
        R"({"name":"tags","type":"array","minItems":2,"maxItems":4,"items":{"type":"string","pattern":"[a-z]{3}"}})");
    for (int i = 0; i < 30; ++i) {
        JsonValue arr = registry.Resolve("array").Generate(field, rng, registry);
        DDG_CHECK(arr.Size() >= 2 && arr.Size() <= 4);
        for (size_t j = 0; j < arr.Size(); ++j) {
            DDG_CHECK_EQ(arr.At(j).AsString().size(), static_cast<size_t>(3));
        }
    }
}

DDG_TEST(ObjectGenerator_BuildsNestedFields) {
    GeneratorRegistry registry;
    RandomEngine rng(9);
    FieldSchema field = MakeField(R"({
        "name":"customer","type":"object",
        "fields":[
            {"name":"company","type":"string","pattern":"[A-Z]{3}"},
            {"name":"vip","type":"bool","trueProbability":1.0}
        ]
    })");
    JsonValue obj = registry.Resolve("object").Generate(field, rng, registry);
    DDG_CHECK(obj.At("company").AsString().size() == 3);
    DDG_CHECK(obj.At("vip").AsBool() == true);
}

DDG_TEST(GenerateFieldValue_NullableFieldCanProduceBothNullAndValue) {
    GeneratorRegistry registry;
    RandomEngine rng(10);
    FieldSchema field = MakeField(R"({"name":"note","type":"int","nullable":true,"nullProbability":0.5})");
    bool sawNull = false, sawValue = false;
    for (int i = 0; i < 200; ++i) {
        JsonValue v = GenerateFieldValue(field, rng, registry);
        if (v.IsNull()) sawNull = true; else sawValue = true;
    }
    DDG_CHECK(sawNull && sawValue);
}

DDG_TEST(GeneratorRegistry_CustomLambdaGeneratorIsResolvable) {
    GeneratorRegistry registry;
    registry.Register("upperFixed", [](const FieldSchema&, RandomEngine&, const GeneratorRegistry&) {
        return JsonValue(std::string("FIXED"));
    });
    RandomEngine rng(11);
    FieldSchema field = MakeField(R"({"name":"x","type":"upperFixed"})");
    DDG_CHECK_EQ(GenerateFieldValue(field, rng, registry).AsString(), std::string("FIXED"));
}

DDG_TEST(GeneratorRegistry_UnknownTypeThrows) {
    GeneratorRegistry registry;
    RandomEngine rng(12);
    FieldSchema field = MakeField(R"({"name":"x","type":"doesNotExist"})");
    DDG_CHECK_THROWS(GenerateFieldValue(field, rng, registry));
}
