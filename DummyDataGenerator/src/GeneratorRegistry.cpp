#include "ddg/GeneratorRegistry.h"

#include "ddg/Generators.h"

namespace ddg {

GeneratorRegistry::GeneratorRegistry() {
    Register("int", std::make_shared<IntGenerator>());
    Register("integer", std::make_shared<IntGenerator>());
    Register("double", std::make_shared<DoubleGenerator>());
    Register("float", std::make_shared<DoubleGenerator>());
    Register("number", std::make_shared<DoubleGenerator>());
    Register("bool", std::make_shared<BoolGenerator>());
    Register("boolean", std::make_shared<BoolGenerator>());
    Register("string", std::make_shared<StringGenerator>());
    Register("enum", std::make_shared<EnumGenerator>());
    Register("date", std::make_shared<DateGenerator>());
    Register("array", std::make_shared<ArrayGenerator>());
    Register("object", std::make_shared<ObjectGenerator>());
}

void GeneratorRegistry::Register(const std::string& typeName, std::shared_ptr<IValueGenerator> generator) {
    if (!generator) {
        throw std::invalid_argument("GeneratorRegistry::Register: generator must not be null");
    }
    generators_[typeName] = std::move(generator);
}

void GeneratorRegistry::Register(const std::string& typeName, LambdaValueGenerator::Func fn) {
    Register(typeName, std::make_shared<LambdaValueGenerator>(std::move(fn)));
}

bool GeneratorRegistry::IsRegistered(const std::string& typeName) const {
    return generators_.find(typeName) != generators_.end();
}

const IValueGenerator& GeneratorRegistry::Resolve(const std::string& typeName) const {
    auto it = generators_.find(typeName);
    if (it == generators_.end()) {
        throw UnknownFieldTypeException("No generator registered for field type '" + typeName + "'");
    }
    return *it->second;
}

json::JsonValue GenerateFieldValue(const FieldSchema& field, RandomEngine& rng, const GeneratorRegistry& registry) {
    if (field.IsNullable() && rng.NextBool(field.GetNullProbability())) {
        return json::JsonValue(nullptr);
    }
    return registry.Resolve(field.GetType()).Generate(field, rng, registry);
}

} // namespace ddg
