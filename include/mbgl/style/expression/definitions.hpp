#include <mbgl/style/expression/expression.hpp>

namespace mbgl {
namespace style {
namespace expression {


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
    static type::StringType type() { return type::String; };
    static std::vector<Params> signatures() {
        return {{type::Value}};
    };
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Get : public LambdaBase<Get> {
public:
    using LambdaBase::LambdaBase;
    static type::ValueType type() { return type::Value; };
    static std::vector<Params> signatures() {
        return {{type::String, NArgs { {type::Object}, 1 }}};
    };
    bool isFeatureConstant() const override;
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Plus : public LambdaBase<Plus> {
public:
    using LambdaBase::LambdaBase;
    static type::NumberType type() { return type::Number; };
    static std::vector<Params> signatures() {
        return {{NArgs {{type::Number}, {}}}};
    };
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Times : public LambdaBase<Times> {
public:
    using LambdaBase::LambdaBase;
    static type::NumberType type() { return type::Number; };
    static std::vector<Params> signatures() {
        return {{NArgs {{type::Number}, {}}}};
    };
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Minus : public LambdaBase<Minus> {
public:
    using LambdaBase::LambdaBase;
    static type::NumberType type() { return type::Number; };
    static std::vector<Params> signatures() {
        return {{type::Number, type::Number}};
    };
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};

class Divide : public LambdaBase<Divide> {
public:
    using LambdaBase::LambdaBase;
    static type::NumberType type() { return type::Number; };
    static std::vector<Params> signatures() {
        return {{type::Number, type::Number}};
    };
    EvaluationResult evaluate(const EvaluationParameters& params) const override;
};


} // namespace expression
} // namespace style
} // namespace mbgl
