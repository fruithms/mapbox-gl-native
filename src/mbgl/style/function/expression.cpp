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

const std::string PlusExpression::name = "+";
const type::Type PlusExpression::type = type::Number;
const std::vector<LambdaExpression::Params> PlusExpression::signatures = {{NArgs {{type::Number}, {}}}};
EvaluationResult PlusExpression::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo + next; });
}

const std::string TimesExpression::name = "*";
const type::Type TimesExpression::type = type::Number;
const std::vector<LambdaExpression::Params> TimesExpression::signatures = {{NArgs {{type::Number}, {}}}};;
EvaluationResult TimesExpression::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo * next; });
}

const std::string MinusExpression::name = "-";
const type::Type MinusExpression::type = type::Number;
const std::vector<LambdaExpression::Params> MinusExpression::signatures = {{type::Number, type::Number}};
EvaluationResult MinusExpression::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo - next; });
}

const std::string DivideExpression::name = "/";
const type::Type DivideExpression::type = type::Number;
const std::vector<LambdaExpression::Params> DivideExpression::signatures = {{type::Number, type::Number}};
EvaluationResult DivideExpression::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo / next; });
}




} // namespace expression
} // namespace style
} // namespace mbgl
