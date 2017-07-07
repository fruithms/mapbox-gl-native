#pragma once

#include <vector>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/variant.hpp>

namespace mbgl {
namespace style {
namespace expression {
namespace type {

class Primitive;
class Variant;
class Array;
class NArgs;
class Typename;
class Lambda;

using ValueType = variant<
    Primitive,
    Typename,
    mapbox::util::recursive_wrapper<Variant>,
    mapbox::util::recursive_wrapper<Array>,
    mapbox::util::recursive_wrapper<NArgs>>;
    
using Type = variant<
    Primitive,
    Typename,
    Variant,
    Array,
    NArgs,
    Lambda>;

class Primitive {
public:
    std::string getName() const { return name; }
    
    static Primitive Null;
    static Primitive Number;
    static Primitive String;
    static Primitive Color;
    static Primitive Object;
    
    // It's weird for this to be on Primitive.  Where should it go?
    static Type Value;
    
private:
    std::string name;
    Primitive(std::string name_) : name(name_) {}
};

class Typename {
public:
    Typename(std::string name_) : name(name_) {}
    std::string getName() const { return name; }
private:
    std::string name;
};

class Array {
public:
    Array(ValueType itemType_) : itemType(itemType_) {}
    Array(ValueType itemType_, int N_) : itemType(itemType_), N(N_) {}
    std::string getName() const {
        return "array";
    }

private:
    ValueType itemType;
    optional<int> N;
};

class Variant {
public:
    Variant(std::vector<ValueType> members_) : members(members_) {}
    std::string getName() const {
        return "variant";
    }
    
    std::vector<ValueType> getMembers() {
        return members;
    }

private:
    std::vector<ValueType> members;
};

class NArgs {
public:
    NArgs(std::vector<ValueType> types_) : types(types_) {}
    std::string getName() const {
        return "nargs";
    }
private:
    std::vector<ValueType> types;
};

class Lambda {
public:
    Lambda(ValueType result_, std::vector<ValueType> params_) :
        result(result_),
        params(params_)
    {}
    std::string getName() const {
        return "lambda";
    }
    ValueType getResult() const { return result; }
private:
    ValueType result;
    std::vector<ValueType> params;
};


} // namespace type
} // namespace expression
} // namespace style
} // namespace mbgl
