#pragma once

#include <array>
#include <vector>
#include <memory>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/variant.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/style/function/type.hpp>
#include <mbgl/util/feature.hpp>
#include <mbgl/style/expression/parsing_context.hpp>
#include <mbgl/style/conversion.hpp>


namespace mbgl {

class GeometryTileFeature;


namespace style {
namespace expression {

struct Value;
using ValueBase = variant<
    NullValue,
    float,
    std::string,
    mbgl::Color,
    mapbox::util::recursive_wrapper<std::vector<Value>>,
    mapbox::util::recursive_wrapper<std::unordered_map<std::string, Value>>>;
struct Value : ValueBase {
    using ValueBase::ValueBase;
};

constexpr NullValue Null = NullValue();

struct EvaluationError {
    std::string message;
};

struct EvaluationParameters {
    float zoom;
    const GeometryTileFeature& feature;
};

using EvaluationResult = variant<EvaluationError, Value>;

class Expression {
public:
    Expression(std::string key_, type::Type type_) : key(key_), type(type_) {}
    virtual ~Expression() {}
    
    virtual EvaluationResult evaluate(const EvaluationParameters&) const = 0;
    
    // Exposed for use with pure Feature objects (e.g. beyond the context of tiled map data).
    EvaluationResult evaluate(float, const Feature&) const;
    
    type::Type getType() const {
        return type;
    }
    
    type::ValueType getResultType() const {
        return type.match(
            [&] (const type::Lambda& lambdaType) {
                return lambdaType.getResult();
            },
            [&] (const auto& t) -> type::ValueType { return t; }
        );
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

struct CompileError {
    std::string message;
    std::string key;
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
    LiteralExpression(std::string key, const NullValue&) : Expression(key, type::Primitive::Null), value(Null) {}

    EvaluationResult evaluate(const EvaluationParameters&) const override {
        return value;
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        if (isUndefined(value))
            return std::make_unique<LiteralExpression>(ctx.key(), Null);
        
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
    Value value;
};

class LambdaExpression : public Expression {
public:
    using Args = std::vector<std::unique_ptr<Expression>>;

    LambdaExpression(std::string key,
                    std::string name_,
                    Args args_,
                    type::Lambda type) :
        Expression(key, type),
        args(std::move(args_)),
        name(name_)
    {}
    
    template <class Expr, class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        assert(isArray(value));
        auto length = arrayLength(value);
        Args args;
        for(size_t i = 1; i < length; i++) {
            const auto& arg = arrayMember(value, i);
            auto parsedArg = parseExpression(arg, ParsingContext(ctx, i, {}));
            if (parsedArg.template is<std::unique_ptr<Expression>>()) {
                args.push_back(std::move(parsedArg.template get<std::unique_ptr<Expression>>()));
            } else {
                return parsedArg.template get<CompileError>();
            }
        }
        return std::make_unique<Expr>(ctx.key(), std::move(args));
    }
    
protected:
    Args args;
private:
    std::string name;
};

template <typename T, typename Rfunc>
EvaluationResult evaluateBinaryOperator(const EvaluationParameters& params,
                                            const LambdaExpression::Args& args,
                                            optional<T> initial,
                                            Rfunc reduce)
{
    optional<T> memo = initial;
    for(const auto& arg : args) {
        auto argValue = arg->evaluate(params);
        if (argValue.is<EvaluationError>()) {
            return argValue.get<EvaluationError>();
        }
        T value = argValue.get<Value>().get<T>();
        if (!memo) memo = {value};
        else memo = reduce(*memo, value);
    }
    return {*memo};
}

class PlusExpression : public LambdaExpression {
public:
    PlusExpression(std::string key, Args args) :
        LambdaExpression(key, "+", std::move(args),
                        {type::Primitive::Number, {type::NArgs({type::Primitive::Number})}})
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo + next; });
    }
};

class TimesExpression : public LambdaExpression {
public:
    TimesExpression(std::string key, Args args) :
        LambdaExpression(key, "*", std::move(args),
                        {type::Primitive::Number, {type::NArgs({type::Primitive::Number})}})
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo * next; });
    }
};

class MinusExpression : public LambdaExpression {
public:
    MinusExpression(std::string key, Args args) :
        LambdaExpression(key, "-", std::move(args),
                        {type::Primitive::Number, {type::Primitive::Number, type::Primitive::Number}})
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo - next; });
    }
};

class DivideExpression : public LambdaExpression {
public:
    DivideExpression(std::string key, Args args) :
        LambdaExpression(key, "/", std::move(args),
                        {type::Primitive::Number, {type::Primitive::Number, type::Primitive::Number}})
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo / next; });
    }
};



} // namespace expression
} // namespace style
} // namespace mbgl
