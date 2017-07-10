#pragma once

#include <vector>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/variant.hpp>

namespace mbgl {
namespace style {
namespace expression {
namespace type {

template <class T>
std::string toString(const T& t);

struct NullType {
    constexpr NullType() {}
    std::string getName() const { return "Null"; }
};

struct NumberType {
    constexpr NumberType() {}
    std::string getName() const { return "Number"; }
};

struct BooleanType {
    constexpr BooleanType() {}
    std::string getName() const { return "Boolean"; }
};

struct StringType {
    constexpr StringType() {}
    std::string getName() const { return "String"; }
};

struct ColorType {
    constexpr ColorType() {}
    std::string getName() const { return "Color"; }
};

struct ObjectType {
    constexpr ObjectType() {}
    std::string getName() const { return "Object"; }
};

struct ValueType {
    constexpr ValueType() {}
    std::string getName() const { return "Value"; }
};

constexpr NullType Null;
constexpr NumberType Number;
constexpr StringType String;
constexpr BooleanType Boolean;
constexpr ColorType Color;
constexpr ValueType Value;
constexpr ObjectType Object;

class Typename {
public:
    Typename(std::string name_) : name(name_) {}
    std::string getName() const { return name; }
private:
    std::string name;
};

class Array;

using Type = variant<
    NullType,
    NumberType,
    BooleanType,
    StringType,
    ColorType,
    ObjectType,
    ValueType,
    Typename,
    mapbox::util::recursive_wrapper<Array>>;

class Array {
public:
    Array(Type itemType_) : itemType(itemType_) {}
    Array(Type itemType_, std::size_t N_) : itemType(itemType_), N(N_) {}
    std::string getName() const {
        if (N) {
            return "Array<" + toString(itemType) + ", " + std::to_string(*N) + ">";
        } else if (toString(itemType) == "Value") {
            return "Array";
        } else {
            return "Array<" + toString(itemType) + ">";
        }
    }

private:
    Type itemType;
    optional<int> N;
};

template <class T>
std::string toString(const T& t) { return t.match([&] (const auto& t) { return t.getName(); }); }


} // namespace type
} // namespace expression
} // namespace style
} // namespace mbgl
