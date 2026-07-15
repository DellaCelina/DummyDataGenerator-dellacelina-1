#include "TestFramework.h"
#include "ddg/Json.h"

using ddg::json::JsonValue;

DDG_TEST(Json_ParsesPrimitives) {
    DDG_CHECK(JsonValue::Parse("null").IsNull());
    DDG_CHECK(JsonValue::Parse("true").AsBool() == true);
    DDG_CHECK(JsonValue::Parse("false").AsBool() == false);
    DDG_CHECK(JsonValue::Parse("42").AsNumber() == 42.0);
    DDG_CHECK(JsonValue::Parse("-3.5").AsNumber() == -3.5);
    DDG_CHECK(JsonValue::Parse("1e2").AsNumber() == 100.0);
    DDG_CHECK_EQ(JsonValue::Parse("\"hello\"").AsString(), std::string("hello"));
}

DDG_TEST(Json_ParsesStringEscapes) {
    JsonValue v = JsonValue::Parse(R"("line1\nline2\t\"quoted\"")");
    DDG_CHECK_EQ(v.AsString(), std::string("line1\nline2\t\"quoted\""));
}

DDG_TEST(Json_ParsesArraysAndObjects) {
    JsonValue v = JsonValue::Parse(R"({"a": 1, "b": [1, 2, 3], "c": {"nested": true}})");
    DDG_CHECK(v.IsObject());
    DDG_CHECK(v.At("a").AsNumber() == 1.0);
    DDG_CHECK_EQ(v.At("b").Size(), static_cast<size_t>(3));
    DDG_CHECK(v.At("b").At(2).AsNumber() == 3.0);
    DDG_CHECK(v.At("c").At("nested").AsBool());
}

DDG_TEST(Json_ContainsReturnsFalseForMissingKey) {
    JsonValue v = JsonValue::Parse(R"({"a": 1})");
    DDG_CHECK(v.Contains("a"));
    DDG_CHECK(!v.Contains("missing"));
}

DDG_TEST(Json_RoundTripsThroughDump) {
    JsonValue original = JsonValue::Parse(R"({"name":"S-001","stock":42,"active":true,"tags":["x","y"],"note":null})");
    std::string compact = original.Dump(-1);
    JsonValue reparsed = JsonValue::Parse(compact);
    DDG_CHECK_EQ(reparsed.At("name").AsString(), std::string("S-001"));
    DDG_CHECK(reparsed.At("stock").AsNumber() == 42.0);
    DDG_CHECK(reparsed.At("active").AsBool());
    DDG_CHECK_EQ(reparsed.At("tags").Size(), static_cast<size_t>(2));
    DDG_CHECK(reparsed.At("note").IsNull());
}

DDG_TEST(Json_DumpIntegersHaveNoDecimalPoint) {
    JsonValue v(static_cast<long long>(42));
    DDG_CHECK_EQ(v.Dump(-1), std::string("42"));
}

DDG_TEST(Json_DumpEscapesSpecialCharacters) {
    JsonValue v(std::string("a\"b\\c\nd"));
    std::string dumped = v.Dump(-1);
    JsonValue reparsed = JsonValue::Parse(dumped);
    DDG_CHECK_EQ(reparsed.AsString(), std::string("a\"b\\c\nd"));
}

DDG_TEST(Json_MalformedInputThrows) {
    DDG_CHECK_THROWS(JsonValue::Parse("{"));
    DDG_CHECK_THROWS(JsonValue::Parse("[1, 2,]"));
    DDG_CHECK_THROWS(JsonValue::Parse("{\"a\": }"));
    DDG_CHECK_THROWS(JsonValue::Parse("nul"));
}

DDG_TEST(Json_BracketOperatorCreatesMissingKey) {
    JsonValue v = JsonValue::MakeObject();
    v["name"] = JsonValue(std::string("item"));
    DDG_CHECK_EQ(v.At("name").AsString(), std::string("item"));
}
