#include <mbgl/style/expression/expression.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>

namespace mbgl {
namespace style {
namespace expression {

class GeoJSONFeature : public GeometryTileFeature {
public:
    const Feature& feature;

    GeoJSONFeature(const Feature& feature_)
        : feature(feature_) {
    }

    FeatureType getType() const override  {
        return apply_visitor(ToFeatureType(), feature.geometry);
    }

    PropertyMap getProperties() const override {
        return feature.properties;
    }

    optional<FeatureIdentifier> getID() const override {
        return feature.id;
    }

    GeometryCollection getGeometries() const override {
        return {};
    }

    optional<mbgl::Value> getValue(const std::string& key) const override {
        auto it = feature.properties.find(key);
        if (it != feature.properties.end()) {
            return optional<mbgl::Value>(it->second);
        }
        return optional<mbgl::Value>();
    }
};



EvaluationResult Expression::evaluate(float z, const Feature& feature) const {
    std::unique_ptr<const GeometryTileFeature> f = std::make_unique<const GeoJSONFeature>(feature);
    return this->evaluate(EvaluationParameters {z, *f});
}


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

template<typename EvalFunc>
EvaluationResult evaluateFromArgs(const EvaluationParameters& params,
                                 const LambdaExpression::Args& args,
                                 EvalFunc evaluate)
{
    std::vector<Value> argValues;
    for(const auto& arg : args) {
        auto argValue = arg->evaluate(params);
        if (argValue.is<EvaluationError>()) {
            return argValue.get<EvaluationError>();
        }
        argValues.emplace_back(argValue.get<Value>());
    }
    return evaluate(argValues);
}

template<typename T, typename EvalFunc>
EvaluationResult evaluateFromArgs(const EvaluationParameters& params,
                                 const std::unique_ptr<Expression>& a0,
                                 EvalFunc evaluate)
{
    const auto& a0value = a0->evaluate<T>(params);
    if (a0value.template is<EvaluationError>()) {
        return a0value.template get<EvaluationError>();
    }
    return evaluate(a0value.template get<T>());
}

template<typename T, typename U, typename EvalFunc>
EvaluationResult evaluateFromArgs(const EvaluationParameters& params,
                                 const std::unique_ptr<Expression>& a0,
                                 const std::unique_ptr<Expression>& a1,
                                 EvalFunc evaluate)
{
    const auto& a0value = a0->evaluate<T>(params);
    if (a0value.template is<EvaluationError>()) {
        return a0value.template get<EvaluationError>();
    }
    const auto& a1value = a1->evaluate<U>(params);
    if (a1value.template is<EvaluationError>()) {
        return a1value.template get<EvaluationError>();
    }
    return evaluate(a0value.template get<T>(), a1value.template get<U>());
}


const std::string TypeOf::name = "typeof";
const type::Type TypeOf::type = type::String;
const std::vector<LambdaExpression::Params> TypeOf::signatures = {{type::Value}};
EvaluationResult TypeOf::evaluate(const EvaluationParameters& params) const  {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&](const Value& v) -> EvaluationResult { return toString(typeOf(v)); },
        [&](const EvaluationError& err) -> EvaluationResult { return err; }
    );
}

const std::string Get::name = "get";
const type::Type Get::type = type::String;
const std::vector<LambdaExpression::Params> Get::signatures = {{type::String, NArgs { {type::Object}, 1 }}};
bool Get::isFeatureConstant() const {
    return args.size() == 1 ? false : LambdaExpression::isFeatureConstant();
}
EvaluationResult Get::evaluate(const EvaluationParameters& params) const {
    if (args.size() == 1) {
        return evaluateFromArgs<std::string>(params, args[0], [&] (const std::string& key) -> Value {
            const auto& value = params.feature.getValue(key);
            if (!value) return Null;
            return convertValue(*value);
        });
    } else {
        return evaluateFromArgs<std::string, std::unordered_map<std::string, Value>>(
            params,
            args[0],
            args[1],
            [&] (const std::string& key, const std::unordered_map<std::string, Value>& object) -> Value {
                if (object.find(key) == object.end()) return Null;
                return object.at(key);
            }
        );
    }
}


const std::string Plus::name = "+";
const type::Type Plus::type = type::Number;
const std::vector<LambdaExpression::Params> Plus::signatures = {{NArgs {{type::Number}, {}}}};
EvaluationResult Plus::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo + next; });
}

const std::string Times::name = "*";
const type::Type Times::type = type::Number;
const std::vector<LambdaExpression::Params> Times::signatures = {{NArgs {{type::Number}, {}}}};;
EvaluationResult Times::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo * next; });
}

const std::string Minus::name = "-";
const type::Type Minus::type = type::Number;
const std::vector<LambdaExpression::Params> Minus::signatures = {{type::Number, type::Number}};
EvaluationResult Minus::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo - next; });
}

const std::string Divide::name = "/";
const type::Type Divide::type = type::Number;
const std::vector<LambdaExpression::Params> Divide::signatures = {{type::Number, type::Number}};
EvaluationResult Divide::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo / next; });
}




} // namespace expression
} // namespace style
} // namespace mbgl
