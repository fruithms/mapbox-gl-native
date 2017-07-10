#pragma once

#include <array>
#include <vector>
#include <memory>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/variant.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/style/expression/type.hpp>
#include <mbgl/style/expression/value.hpp>
#include <mbgl/style/expression/parsing_context.hpp>
#include <mbgl/style/conversion.hpp>


namespace mbgl {

class GeometryTileFeature;


namespace style {
namespace expression {

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
    LiteralExpression(std::string key, type::Type type, Value value_) : Expression(key, type), value(value_) {}

    EvaluationResult evaluate(const EvaluationParameters&) const override {
        return value;
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        const Value& parsedValue = parseValue(value);
        const type::Type& type = typeOf(parsedValue);
        return std::make_unique<LiteralExpression>(ctx.key(), type, parsedValue);
    }
    
private:
    template <class V>
    static Value parseValue(const V& value) {
        if (isUndefined(value)) return Null;
        if (isObject(value)) {
            std::unordered_map<std::string, Value> result;
            eachMember(value, [&] (const std::string& k, const V& v) -> optional<conversion::Error> {
                result.emplace(k, parseValue(v));
                return {};
            });
            return result;
        }
        if (isArray(value)) {
            std::vector<Value> result;
            const auto length = arrayLength(value);
            for(std::size_t i = 0; i < length; i++) {
                result.emplace_back(parseValue(arrayMember(value, i)));
            }
            return result;
        }
        
        optional<mbgl::Value> v = toValue(value);
        assert(v);
        return v->match(
            [&] (const std::string& s) -> Value { return s; },
            [&] (bool b) -> Value { return b; },
            [&] (auto f) -> Value { return *numericValue<float>(f); }
        );
    }

    Value value;
};

struct NArgs {
    std::vector<type::Type> types;
    optional<std::size_t> N;
};

class LambdaExpression : public Expression {
public:
    using Args = std::vector<std::unique_ptr<Expression>>;

    LambdaExpression(std::string key,
                    std::string name_,
                    Args args_,
                    type::Type type) :
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
        LambdaExpression(key, "+", std::move(args), type::Number)
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo + next; });
    }
};

class TimesExpression : public LambdaExpression {
public:
    TimesExpression(std::string key, Args args) :
        LambdaExpression(key, "*", std::move(args), type::Number)
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo * next; });
    }
};

class MinusExpression : public LambdaExpression {
public:
    MinusExpression(std::string key, Args args) :
        LambdaExpression(key, "-", std::move(args), type::Number)
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo - next; });
    }
};

class DivideExpression : public LambdaExpression {
public:
    DivideExpression(std::string key, Args args) :
        LambdaExpression(key, "/", std::move(args), type::Number)
    {}
    
    EvaluationResult evaluate(const EvaluationParameters& params) const override {
        return evaluateBinaryOperator<float>(params, args,
            {}, [](float memo, float next) { return memo / next; });
    }
};



} // namespace expression
} // namespace style
} // namespace mbgl
