#pragma once

#include <mbgl/util/color.hpp>
#include <mbgl/util/variant.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/style/expression/type.hpp>

namespace mbgl {
namespace style {
namespace expression {

struct Value;
using ValueBase = variant<
    NullValue,
    bool,
    float,
    std::string,
    mbgl::Color,
    mapbox::util::recursive_wrapper<std::vector<Value>>,
    mapbox::util::recursive_wrapper<std::unordered_map<std::string, Value>>>;
struct Value : ValueBase {
    using ValueBase::ValueBase;
};

constexpr NullValue Null = NullValue();

Value convertValue(const mbgl::Value&);
type::Type typeOf(const Value& value);

template <typename T>
std::string valueTypeToString();


} // namespace expression
} // namespace style
} // namespace mbgl
