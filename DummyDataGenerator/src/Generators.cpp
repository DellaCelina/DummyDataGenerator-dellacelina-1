#include "ddg/Generators.h"

#include "ddg/GeneratorRegistry.h"
#include "ddg/RegexStringGenerator.h"

#include <cmath>
#include <sstream>
#include <iomanip>
#include <string>

namespace ddg {

// ---------------------------------------------------------------------------
// IntGenerator
// ---------------------------------------------------------------------------
json::JsonValue IntGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry&) const {
    long long min = field.GetInt("min", 0);
    long long max = field.GetInt("max", 100);
    if (min > max) {
        throw SchemaException("Field '" + field.GetName() + "': 'min' must be <= 'max'");
    }
    return json::JsonValue(static_cast<long long>(rng.NextInt(min, max)));
}

// ---------------------------------------------------------------------------
// DoubleGenerator
// ---------------------------------------------------------------------------
json::JsonValue DoubleGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry&) const {
    double min = field.GetDouble("min", 0.0);
    double max = field.GetDouble("max", 1.0);
    if (min > max) {
        throw SchemaException("Field '" + field.GetName() + "': 'min' must be <= 'max'");
    }
    double value = rng.NextDouble(min, max);
    if (field.Has("decimalPlaces")) {
        long long places = field.GetInt("decimalPlaces", 2);
        if (places < 0) {
            throw SchemaException("Field '" + field.GetName() + "': 'decimalPlaces' must be >= 0");
        }
        double factor = std::pow(10.0, static_cast<double>(places));
        value = std::round(value * factor) / factor;
    }
    return json::JsonValue(value);
}

// ---------------------------------------------------------------------------
// BoolGenerator
// ---------------------------------------------------------------------------
json::JsonValue BoolGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry&) const {
    double trueProbability = field.GetDouble("trueProbability", 0.5);
    return json::JsonValue(rng.NextBool(trueProbability));
}

// ---------------------------------------------------------------------------
// StringGenerator
// ---------------------------------------------------------------------------
json::JsonValue StringGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry&) const {
    if (field.Has("pattern")) {
        std::string pattern = field.GetString("pattern", "");
        long long maxRepeat = field.GetInt("regexMaxRepeat", 6);
        return json::JsonValue(RegexStringGenerator::GenerateFromPattern(pattern, rng, static_cast<int>(maxRepeat)));
    }

    static const std::string kDefaultCharset =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string charset = field.GetString("charset", kDefaultCharset);
    if (charset.empty()) {
        throw SchemaException("Field '" + field.GetName() + "': 'charset' must not be empty");
    }

    long long length;
    if (field.Has("length")) {
        length = field.GetInt("length", 0);
        if (length < 0) {
            throw SchemaException("Field '" + field.GetName() + "': 'length' must be >= 0");
        }
    } else {
        long long minLength = field.GetInt("minLength", 5);
        long long maxLength = field.GetInt("maxLength", 10);
        if (minLength < 0 || minLength > maxLength) {
            throw SchemaException("Field '" + field.GetName() + "': 'minLength' must be >= 0 and <= 'maxLength'");
        }
        length = rng.NextInt(minLength, maxLength);
    }

    std::string out;
    out.reserve(static_cast<size_t>(length));
    for (long long i = 0; i < length; ++i) {
        out += charset[rng.NextIndex(charset.size())];
    }
    return json::JsonValue(out);
}

// ---------------------------------------------------------------------------
// EnumGenerator
// ---------------------------------------------------------------------------
json::JsonValue EnumGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry&) const {
    const auto& valuesNode = field.Require("values");
    if (!valuesNode.IsArray() || valuesNode.Size() == 0) {
        throw SchemaException("Field '" + field.GetName() + "' (type 'enum') requires a non-empty 'values' array");
    }
    size_t idx = rng.NextIndex(valuesNode.Size());
    return valuesNode.At(idx);
}

// ---------------------------------------------------------------------------
// DateGenerator
// ---------------------------------------------------------------------------
namespace {

// Howard Hinnant's civil_from_days / days_from_civil algorithms. These give
// a dependency-free, timezone-free way to do date arithmetic without relying
// on platform-specific <ctime> behavior.
long long DaysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    long long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = static_cast<unsigned>(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

void CivilFromDays(long long z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = static_cast<unsigned>(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long yyyy = static_cast<long long>(yoe) + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3 : static_cast<unsigned>(-9));
    y = static_cast<int>(yyyy + (m <= 2 ? 1 : 0));
}

void ParseIsoDate(const std::string& s, const std::string& fieldName, int& y, unsigned& m, unsigned& d) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') {
        throw SchemaException("Field '" + fieldName + "': date '" + s + "' must be in 'YYYY-MM-DD' format");
    }
    try {
        y = std::stoi(s.substr(0, 4));
        m = static_cast<unsigned>(std::stoi(s.substr(5, 2)));
        d = static_cast<unsigned>(std::stoi(s.substr(8, 2)));
    } catch (const std::exception&) {
        throw SchemaException("Field '" + fieldName + "': date '" + s + "' must be in 'YYYY-MM-DD' format");
    }
    if (m < 1 || m > 12 || d < 1 || d > 31) {
        throw SchemaException("Field '" + fieldName + "': date '" + s + "' is out of range");
    }
}

std::string FormatDate(int y, unsigned m, unsigned d, const std::string& format) {
    std::string out;
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%' && i + 1 < format.size()) {
            char token = format[i + 1];
            std::ostringstream part;
            switch (token) {
                case 'Y': part << std::setw(4) << std::setfill('0') << y; break;
                case 'm': part << std::setw(2) << std::setfill('0') << m; break;
                case 'd': part << std::setw(2) << std::setfill('0') << d; break;
                case '%': part << '%'; break;
                default: part << '%' << token; break;
            }
            out += part.str();
            ++i;
        } else {
            out += format[i];
        }
    }
    return out;
}

} // namespace

json::JsonValue DateGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry&) const {
    int minY, maxY;
    unsigned minM, minD, maxM, maxD;
    ParseIsoDate(field.GetString("min", "1970-01-01"), field.GetName(), minY, minM, minD);
    ParseIsoDate(field.GetString("max", "2038-01-01"), field.GetName(), maxY, maxM, maxD);

    long long minDays = DaysFromCivil(minY, minM, minD);
    long long maxDays = DaysFromCivil(maxY, maxM, maxD);
    if (minDays > maxDays) {
        throw SchemaException("Field '" + field.GetName() + "': date 'min' must be <= 'max'");
    }

    long long days = rng.NextInt(minDays, maxDays);
    int y;
    unsigned m, d;
    CivilFromDays(days, y, m, d);

    std::string format = field.GetString("format", "%Y-%m-%d");
    return json::JsonValue(FormatDate(y, m, d, format));
}

// ---------------------------------------------------------------------------
// ArrayGenerator
// ---------------------------------------------------------------------------
json::JsonValue ArrayGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const {
    const auto& itemsNode = field.Require("items");
    if (!itemsNode.IsObject()) {
        throw SchemaException("Field '" + field.GetName() + "' (type 'array'): 'items' must be a field-definition object");
    }
    json::JsonValue itemNode = itemsNode;
    if (!itemNode.Contains("name")) {
        itemNode["name"] = json::JsonValue(std::string("item"));
    }
    FieldSchema itemSchema(itemNode);

    long long minItems = field.GetInt("minItems", 0);
    long long maxItems = field.GetInt("maxItems", minItems + 3);
    if (minItems < 0 || minItems > maxItems) {
        throw SchemaException("Field '" + field.GetName() + "': 'minItems' must be >= 0 and <= 'maxItems'");
    }

    long long count = rng.NextInt(minItems, maxItems);
    json::JsonValue::ArrayType arr;
    arr.reserve(static_cast<size_t>(count));
    for (long long i = 0; i < count; ++i) {
        arr.push_back(GenerateFieldValue(itemSchema, rng, registry));
    }
    return json::JsonValue(std::move(arr));
}

// ---------------------------------------------------------------------------
// ObjectGenerator
// ---------------------------------------------------------------------------
json::JsonValue ObjectGenerator::Generate(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) const {
    const auto& fieldsNode = field.Require("fields");
    if (!fieldsNode.IsArray()) {
        throw SchemaException("Field '" + field.GetName() + "' (type 'object'): 'fields' must be an array");
    }
    json::JsonValue::ObjectType obj;
    for (const auto& subNode : fieldsNode.AsArray()) {
        FieldSchema subField(subNode);
        obj.emplace_back(subField.GetName(), GenerateFieldValue(subField, rng, registry));
    }
    return json::JsonValue(std::move(obj));
}

} // namespace ddg
