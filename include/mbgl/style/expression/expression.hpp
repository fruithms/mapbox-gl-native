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

struct CompileError {
    std::string message;
    std::string key;
};

class Expression {
public:
    Expression(std::string key_, type::Type type_) : key(key_), type(type_) {}
    virtual ~Expression() {};
    
    virtual EvaluationResult evaluate(const EvaluationParameters& params) const = 0;
    
    template <typename T>
    variant<EvaluationError, T> evaluate(const EvaluationParameters& params) {
        const auto& result = evaluate(params);
        return result.match(
            [&] (const EvaluationError& err) -> variant<EvaluationError, T> {
                return err;
            },
            [&] (const Value& value) {
                return value.match(
                    [&] (const T& v) -> variant<EvaluationError, T> { return v; },
                    [&] (const auto& v) -> variant<EvaluationError, T> {
                        return EvaluationError {
                            "Expected " + valueTypeToString<T>() + " but found " + toString(typeOf(v)) + " instead."
                        };
                    }
                );
            }
        );
    }
    
    EvaluationResult evaluate(float z, const Feature& feature) const;
    
    type::Type getType() const { return type; }
    std::string getKey() const { return key; }
    
    virtual bool isFeatureConstant() const { return true; }
    virtual bool isZoomConstant() const { return true; }
    
private:
    std::string key;
    type::Type type;
};


using ParseResult = variant<CompileError, std::unique_ptr<Expression>>;
template <class V>
ParseResult parseExpression(const V& value, const ParsingContext& context);

using TypecheckResult = variant<std::vector<CompileError>, std::unique_ptr<Expression>>;

using namespace mbgl::style::conversion;

class LiteralExpression : public Expression {
public:
    LiteralExpression(std::string key_, type::Type type_, Value value_) : Expression(key_, type_), value(value_) {}
    
    Value getValue() const { return value; }

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
        return convertValue(*v);
    }

    Value value;
};

struct NArgs {
    std::vector<type::Type> types;
    optional<std::size_t> N;
};

class LambdaExpression : public Expression {
public:
    using Params = std::vector<variant<type::Type, NArgs>>;
    using Args = std::vector<std::unique_ptr<Expression>>;

    LambdaExpression(std::string key_,
                    std::string name_,
                    Args args_,
                    type::Type type_,
                    std::vector<Params> overloads_) :
        Expression(key_, type_),
        args(std::move(args_)),
        overloads(overloads_),
        name(name_)
    {}
    
    std::string getName() const { return name; }
    
    virtual bool isFeatureConstant() const override {
        bool isFC = true;
        for (const auto& arg : args) {
            isFC = isFC && arg->isFeatureConstant();
        }
        return isFC;
    }
    
    virtual bool isZoomConstant() const override {
        bool isZC = true;
        for (const auto& arg : args) {
            isZC = isZC && arg->isZoomConstant();
        }
        return isZC;
    }
    
    virtual std::unique_ptr<Expression> applyInferredType(const type::Type& type, Args args) const = 0;
    
    friend TypecheckResult typecheck(const type::Type& expected, const std::unique_ptr<Expression>& e);

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
    std::vector<Params> overloads;
    std::string name;
};

template<class Expr>
class LambdaBase : public LambdaExpression {
public:
    LambdaBase(const std::string& key, Args args) :
        LambdaExpression(key, Expr::name, std::move(args), Expr::type, Expr::signatures)
    {}
    LambdaBase(const std::string& key, const type::Type& type, Args args) :
        LambdaExpression(key, Expr::name, std::move(args), type, Expr::signatures)
    {}

    std::unique_ptr<Expression> applyInferredType(const type::Type& type, Args args) const override {
        return std::make_unique<Expr>(getKey(), type, std::move(args));
    }
};

// Concrete expression definitions
class MathConstant : public LambdaExpression {
public:
    MathConstant(const std::string& key, const std::string& name, float value_) :
        LambdaExpression(key, name, {}, type::Number, {{}}),
        value(value_)
    {}
    
    EvaluationResult evaluate(const EvaluationParameters&) const override { return value; }
    
    std::unique_ptr<Expression> applyInferredType(const type::Type&, Args) const override {
        return std::make_unique<MathConstant>(getKey(), getName(), value);
    }
    
    // TODO: declaring these constants like `static constexpr double E = 2.718...` caused
    // a puzzling link error.
    static std::unique_ptr<Expression> ln2(const ParsingContext& ctx) {
        return std::make_unique<MathConstant>(ctx.key(), "ln2", 0.693147180559945309417);
    }
    static std::unique_ptr<Expression> e(const ParsingContext& ctx) {
        return std::make_unique<MathConstant>(ctx.key(), "e", 2.71828182845904523536);
    }
    static std::unique_ptr<Expression> pi(const ParsingContext& ctx) {
        return std::make_unique<MathConstant>(ctx.key(), "pi", 3.14159265358979323846);
    }
private:
    float value;
};

class TypeOf : public LambdaBase<TypeOf> {
public:
    using LambdaBase::LambdaBase;
    static const std::string name;
    static const type::Type type;
    static const std::vector<Params> signatures;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Get : public LambdaBase<Get> {
public:
    using LambdaBase::LambdaBase;
    static const std::string name;
    static const type::Type type;
    static const std::vector<Params> signatures;
    bool isFeatureConstant() const override;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Plus : public LambdaBase<Plus> {
public:
    using LambdaBase::LambdaBase;
    static const std::string name;
    static const type::Type type;
    static const std::vector<Params> signatures;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Times : public LambdaBase<Times> {
public:
    using LambdaBase::LambdaBase;
    static const std::string name;
    static const type::Type type;
    static const std::vector<Params> signatures;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Minus : public LambdaBase<Minus> {
public:
    using LambdaBase::LambdaBase;
    static const std::string name;
    static const type::Type type;
    static const std::vector<Params> signatures;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Divide : public LambdaBase<Divide> {
public:
    using LambdaBase::LambdaBase;
    static const std::string name;
    static const type::Type type;
    static const std::vector<Params> signatures;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};



} // namespace expression
} // namespace style
} // namespace mbgl
