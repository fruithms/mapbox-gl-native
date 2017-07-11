#include  <mbgl/style/expression/value.hpp>

namespace mbgl {
namespace style {
namespace expression {

struct ConvertValue {
//    null_value_t, bool, uint64_t, int64_t, double, std::string,
//                                         mapbox::util::recursive_wrapper<std::vector<value>>,
//                                         mapbox::util::recursive_wrapper<std::unordered_map<std::string, value>>
    Value operator()(const std::vector<mbgl::Value>& v) {
        std::vector<Value> result;
        for(const auto& item : v) {
            result.emplace_back(convertValue(item));
        }
        return result;
    }
    
    Value operator()(const std::unordered_map<std::string, mbgl::Value>& v) {
        std::unordered_map<std::string, Value> result;
        for(const auto& entry : v) {
            result.emplace(entry.first, convertValue(entry.second));
        }
        return result;
    }
    
    Value operator()(const std::string& s) { return s; }
    Value operator()(const bool& b) { return b; }
    Value operator()(const mbgl::NullValue) { return Null; }
    
    template <typename T>
    Value operator()(const T& v) { return *numericValue<float>(v); }
};

Value convertValue(const mbgl::Value& value) {
    return mbgl::Value::visit(value, ConvertValue());
}

type::Type typeOf(const Value& value) {
    return value.match(
        [&](bool) -> type::Type { return type::Boolean; },
        [&](float) -> type::Type { return type::Number; },
        [&](const std::string&) -> type::Type { return type::String; },
        [&](const mbgl::Color&) -> type::Type { return type::Color; },
        [&](const NullValue&) -> type::Type { return type::Null; },
        [&](const std::unordered_map<std::string, Value>&) -> type::Type { return type::Object; },
        [&](const std::vector<Value>& arr) -> type::Type {
            optional<type::Type> itemType;
            for (const auto& item : arr) {
                const auto& t = typeOf(item);
                const auto& tname = type::toString(t);
                if (!itemType) {
                    itemType = {t};
                } else if (type::toString(*itemType) == tname) {
                    continue;
                } else {
                    itemType = {type::Value};
                    break;
                }
            }
            
            if (!itemType) { itemType = {type::Value}; }

            return type::Array(*itemType, arr.size());
        }
    );
}

template <> std::string valueTypeToString<Value>() { return "Value"; }
template <> std::string valueTypeToString<NullValue>() { return "Null"; }
template <> std::string valueTypeToString<bool>() { return "Boolean"; }
template <> std::string valueTypeToString<float>() { return "Number"; }
template <> std::string valueTypeToString<std::string>() { return "String"; }
template <> std::string valueTypeToString<mbgl::Color>() { return "Color"; }
template <> std::string valueTypeToString<std::unordered_map<std::string, Value>>() { return "Object"; }
template <> std::string valueTypeToString<std::array<float, 2>>() { return "Array<Number, 2>"; }
template <> std::string valueTypeToString<std::array<float, 4>>() { return "Array<Number, 4>"; }
template <> std::string valueTypeToString<std::vector<Value>>() { return "Array"; }

} // namespace expression
} // namespace style
} // namespace mbgl
