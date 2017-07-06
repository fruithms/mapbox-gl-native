#pragma once

#include <array>
#include <vector>
#include <memory>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/style/function/type.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/style/expression/parsing_context.hpp>
#include <mbgl/style/conversion.hpp>

namespace mbgl {

class GeometryTileFeature;


namespace style {
namespace expression {

using OutputValue = variant<
    float,
    std::string,
    mbgl::Color,
    std::array<float, 2>,
    std::array<float, 4>>;

struct EvaluationError {
    std::string message;
};

struct CompileError {
    std::string message;
    std::string key;
};

class Expression {
public:
    Expression(std::string key_, type::Type type_) : key(key_), type(type_) {}
    virtual ~Expression() {}
    
    virtual optional<OutputValue> evaluate(float, const GeometryTileFeature&, EvaluationError& e) const = 0;
    
    // Exposed for use with pure Feature objects (e.g. beyond the context of tiled map data).
    optional<OutputValue> evaluate(float, const Feature&, EvaluationError&) const;
    
    type::Type getType() {
        return type;
    }
    
    bool isFeatureConstant() {
        return true;
    }
    
    bool isZoomConstant() {
        return true;
    }
    
private:
    std::string key;
    type::Type type;
};

using ParseResult = variant<CompileError, std::unique_ptr<Expression>>;
template <class V>
ParseResult parseExpression(const V& value, const ParsingContext& context);

using namespace mbgl::style::conversion;

class LiteralExpression : public Expression {
public:
    LiteralExpression(std::string key, float value_) : Expression(key, type::Primitive::Number), value(value_) {}
    LiteralExpression(std::string key, const std::string& value_) : Expression(key, type::Primitive::String), value(value_) {}
    LiteralExpression(std::string key, const mbgl::Color& value_) : Expression(key, type::Primitive::Color), value(value_) {}
    LiteralExpression(std::string key) : Expression(key, type::Primitive::Null) {}
    
    optional<OutputValue> evaluate(float, const GeometryTileFeature&, EvaluationError&) const override {
        return value;
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        if (isUndefined(value))
            return std::make_unique<LiteralExpression>(ctx.key());
        
        if (isObject(value)) {
            return CompileError {ctx.key(), "Unimplemented: object literals"};
        }
        
        if (isArray(value)) {
            return CompileError {ctx.key(), "Unimplemented: array literals"};
        }
        
        optional<mbgl::Value> v = toValue(value);
        assert(v);
        return v->match(
            [&] (std::string s) { return std::make_unique<LiteralExpression>(ctx.key(), s); },
            [&] (bool b) { return std::make_unique<LiteralExpression>(ctx.key(), b); },
            [&] (auto f) {
                auto number = numericValue<float>(f);
                assert(number);
                return std::make_unique<LiteralExpression>(ctx.key(), *number);
            }
        );
    }
    
private:
    optional<OutputValue> value;
};

class LambdaExpression : public Expression {
public:
    LambdaExpression(std::string key,
                    type::Lambda type,
                    std::string name_,
                    std::vector<std::unique_ptr<Expression>> args_) :
        Expression(key, type),
        name(name_),
        args(std::move(args_))
    {}
    
private:
    std::string name;
    std::vector<std::unique_ptr<Expression>> args;
};




} // namespace expression
} // namespace style
} // namespace mbgl
