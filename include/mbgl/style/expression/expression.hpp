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
    using Args = std::vector<std::unique_ptr<Expression>>;

    LambdaExpression(std::string key,
                    std::string name_,
                    Args args_,
                    type::Lambda type) :
        Expression(key, type),
        args(std::move(args_)),
        name(name_)
    {}
    
    template <class V>
    static variant<CompileError, Args> parseArgs(const V& value, const ParsingContext& ctx) {
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
        return std::move(args);
    }
    
protected:
    Args args;
private:
    std::string name;
};

template <typename T, typename Rfunc>
optional<OutputValue> evaluateBinaryOperator(float z,
                                            const GeometryTileFeature& f,
                                            EvaluationError& e,
                                            const LambdaExpression::Args& args,
                                            optional<T> initial,
                                            Rfunc reduce)
{
    optional<T> memo = initial;
    for(const auto& arg : args) {
        auto argValue = arg->evaluate(z, f, e);
        if (!argValue) return {};
        if (!memo) memo = {argValue->get<T>()};
        else memo = reduce(*memo, argValue->get<T>());
    }
    return {*memo};
}

class PlusExpression : public LambdaExpression {
public:
    PlusExpression(std::string key, Args args) :
        LambdaExpression(key, "+", std::move(args),
                        {type::Primitive::Number, {type::NArgs({type::Primitive::Number})}})
    {}
    
    optional<OutputValue> evaluate(float zoom, const GeometryTileFeature& feature, EvaluationError& error) const override {
        return evaluateBinaryOperator<float>(zoom, feature, error, args,
            {}, [](float memo, float next) { return memo + next; });
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        auto args = LambdaExpression::parseArgs(value, ctx);
        if (args.template is<LambdaExpression::Args>()) {
            return std::make_unique<PlusExpression>(ctx.key(), std::move(args.template get<Args>()));
        } else {
            return args.template get<CompileError>();
        }
    }
};

class TimesExpression : public LambdaExpression {
public:
    TimesExpression(std::string key, Args args) :
        LambdaExpression(key, "*", std::move(args),
                        {type::Primitive::Number, {type::NArgs({type::Primitive::Number})}})
    {}
    
    optional<OutputValue> evaluate(float zoom, const GeometryTileFeature& feature, EvaluationError& error) const override {
        return evaluateBinaryOperator<float>(zoom, feature, error, args,
            {}, [](float memo, float next) { return memo * next; });
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        auto args = LambdaExpression::parseArgs(value, ctx);
        if (args.template is<LambdaExpression::Args>()) {
            return std::make_unique<TimesExpression>(ctx.key(), std::move(args.template get<Args>()));
        } else {
            return args.template get<CompileError>();
        }
    }
};

class MinusExpression : public LambdaExpression {
public:
    MinusExpression(std::string key, Args args) :
        LambdaExpression(key, "-", std::move(args),
                        {type::Primitive::Number, {type::Primitive::Number, type::Primitive::Number}})
    {}
    
    optional<OutputValue> evaluate(float zoom, const GeometryTileFeature& feature, EvaluationError& error) const override {
        return evaluateBinaryOperator<float>(zoom, feature, error, args,
            {}, [](float memo, float next) { return memo - next; });
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        auto args = LambdaExpression::parseArgs(value, ctx);
        if (args.template is<LambdaExpression::Args>()) {
            return std::make_unique<MinusExpression>(ctx.key(), std::move(args.template get<Args>()));
        } else {
            return args.template get<CompileError>();
        }
    }
};

class DivideExpression : public LambdaExpression {
public:
    DivideExpression(std::string key, Args args) :
        LambdaExpression(key, "/", std::move(args),
                        {type::Primitive::Number, {type::Primitive::Number, type::Primitive::Number}})
    {}
    
    optional<OutputValue> evaluate(float zoom, const GeometryTileFeature& feature, EvaluationError& error) const override {
        return evaluateBinaryOperator<float>(zoom, feature, error, args,
            {}, [](float memo, float next) { return memo / next; });
    }
    
    template <class V>
    static ParseResult parse(const V& value, const ParsingContext& ctx) {
        auto args = LambdaExpression::parseArgs(value, ctx);
        if (args.template is<LambdaExpression::Args>()) {
            return std::make_unique<DivideExpression>(ctx.key(), std::move(args.template get<Args>()));
        } else {
            return args.template get<CompileError>();
        }
    }
};



} // namespace expression
} // namespace style
} // namespace mbgl
