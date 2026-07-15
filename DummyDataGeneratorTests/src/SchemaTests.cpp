#include "TestFramework.h"
#include "ddg/Schema.h"

using ddg::FieldSchema;
using ddg::Schema;
using ddg::SchemaException;

DDG_TEST(FieldSchema_RequiresNameAndType) {
    DDG_CHECK_THROWS(FieldSchema(ddg::json::JsonValue::Parse(R"({"type":"int"})")));
    DDG_CHECK_THROWS(FieldSchema(ddg::json::JsonValue::Parse(R"({"name":"x"})")));
}

DDG_TEST(FieldSchema_TypeIsLowercased) {
    FieldSchema field(ddg::json::JsonValue::Parse(R"({"name":"x","type":"INT"})"));
    DDG_CHECK_EQ(field.GetType(), std::string("int"));
}

DDG_TEST(FieldSchema_DefaultsAndOverrides) {
    FieldSchema withDefaults(ddg::json::JsonValue::Parse(R"({"name":"x","type":"int"})"));
    DDG_CHECK(withDefaults.GetInt("min", -1) == -1);
    DDG_CHECK(!withDefaults.IsNullable());
    DDG_CHECK(!withDefaults.IsUnique());

    FieldSchema explicitField(ddg::json::JsonValue::Parse(
        R"({"name":"x","type":"int","min":5,"nullable":true,"nullProbability":0.4,"unique":true})"));
    DDG_CHECK(explicitField.GetInt("min", -1) == 5);
    DDG_CHECK(explicitField.IsNullable());
    DDG_CHECK(explicitField.GetNullProbability() == 0.4);
    DDG_CHECK(explicitField.IsUnique());
}

DDG_TEST(FieldSchema_RequireThrowsWhenMissing) {
    FieldSchema field(ddg::json::JsonValue::Parse(R"({"name":"x","type":"enum"})"));
    DDG_CHECK_THROWS(field.Require("values"));
}

DDG_TEST(Schema_ParsesFieldsArray) {
    Schema schema = Schema::FromString(R"({
        "name": "Widget",
        "fields": [
            { "name": "id", "type": "int" },
            { "name": "label", "type": "string" }
        ]
    })");
    DDG_CHECK_EQ(schema.GetName(), std::string("Widget"));
    DDG_CHECK_EQ(schema.GetFields().size(), static_cast<size_t>(2));
    DDG_CHECK_EQ(schema.GetFields()[0].GetName(), std::string("id"));
}

DDG_TEST(Schema_RejectsMissingOrEmptyFields) {
    DDG_CHECK_THROWS(Schema::FromString(R"({"name":"Widget"})"));
    DDG_CHECK_THROWS(Schema::FromString(R"({"name":"Widget","fields":[]})"));
}

DDG_TEST(Schema_NameIsOptional) {
    Schema schema = Schema::FromString(R"({"fields":[{"name":"id","type":"int"}]})");
    DDG_CHECK(schema.GetName().empty());
}
