#include  <mbgl/style/expression/value.hpp>

namespace mbgl {
namespace style {
namespace expression {

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

} // namespace expression
} // namespace style
} // namespace mbgl
