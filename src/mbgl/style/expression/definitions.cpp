#include <mbgl/style/expression/definitions.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>

namespace mbgl {
namespace style {
namespace expression {

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

// TODO: get this working to replace all the overloads below

//template <typename ...Ts>
//struct restargs { using expression = std::unique_ptr<Expression>; };
//
//template <typename T, typename ...Ts, typename Eval>
//EvaluationResult evaluateFromArgs(const EvaluationParameters& params,
//                                 const std::unique_ptr<Expression>& a0,
//                                 const typename restargs<Ts...>::expression& args,
//                                 Eval evaluate)
//{
//    const auto& a0value = a0->evaluate<T>(params);
//    if (a0value.template is<EvaluationError>()) {
//        return a0value.template get<EvaluationError>();
//    }
//    return evaluateFromArgs<Ts...>(
//        params,
//        std::forward<const typename restargs<Ts...>::expression&>(args),
//        [&] (Ts... restValues) {
//            return evaluate(a0value.template get<T>(), std::forward<Ts...>(restValues)...);
//        }
//    );
//}

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

template<typename T0, typename T1, typename T2, typename EvalFunc>
EvaluationResult evaluateFromArgs(const EvaluationParameters& params,
                                 const std::unique_ptr<Expression>& a0,
                                 const std::unique_ptr<Expression>& a1,
                                 const std::unique_ptr<Expression>& a2,
                                 EvalFunc evaluate)
{
    const auto& a0value = a0->evaluate<T0>(params);
    if (a0value.template is<EvaluationError>()) {
        return a0value.template get<EvaluationError>();
    }
    const auto& a1value = a1->evaluate<T1>(params);
    if (a1value.template is<EvaluationError>()) {
        return a1value.template get<EvaluationError>();
    }
    const auto& a2value = a2->evaluate<T2>(params);
    if (a2value.template is<EvaluationError>()) {
        return a2value.template get<EvaluationError>();
    }
    return evaluate(
        a0value.template get<T0>(),
        a1value.template get<T1>(),
        a2value.template get<T2>()
    );
}

template<typename T0, typename T1, typename T2, typename T3, typename EvalFunc>
EvaluationResult evaluateFromArgs(const EvaluationParameters& params,
                                 const std::unique_ptr<Expression>& a0,
                                 const std::unique_ptr<Expression>& a1,
                                 const std::unique_ptr<Expression>& a2,
                                 const std::unique_ptr<Expression>& a3,
                                 EvalFunc evaluate)
{
    const auto& a0value = a0->evaluate<T0>(params);
    if (a0value.template is<EvaluationError>()) {
        return a0value.template get<EvaluationError>();
    }
    const auto& a1value = a1->evaluate<T1>(params);
    if (a1value.template is<EvaluationError>()) {
        return a1value.template get<EvaluationError>();
    }
    const auto& a2value = a2->evaluate<T2>(params);
    if (a2value.template is<EvaluationError>()) {
        return a2value.template get<EvaluationError>();
    }
    const auto& a3value = a3->evaluate<T3>(params);
    if (a3value.template is<EvaluationError>()) {
        return a3value.template get<EvaluationError>();
    }
    return evaluate(
        a0value.template get<T0>(),
        a1value.template get<T1>(),
        a2value.template get<T2>(),
        a3value.template get<T3>()
    );
}


EvaluationResult TypeOf::evaluate(const EvaluationParameters& params) const  {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&](const Value& v) -> EvaluationResult { return toString(typeOf(v)); },
        [&](const EvaluationError& err) -> EvaluationResult { return err; }
    );
}

EvaluationResult Array::evaluate(const EvaluationParameters& params) const  {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&](const Value& v) -> EvaluationResult {
            const auto& expected = getType().get<type::Array>();
            const auto& actual = typeOf(v);
            if (actual.is<type::Array>()) {
                const auto& arrayType = actual.get<type::Array>();
                bool match = (!expected.N || expected.N == arrayType.N);
                if (expected.itemType.is<type::ValueType>()) {
                    match = match && (arrayType.itemType.is<type::StringType>() ||
                        arrayType.itemType.is<type::NumberType>() ||
                        arrayType.itemType.is<type::BooleanType>());
                } else {
                    match = match && (toString(expected.itemType) == toString(arrayType.itemType));
                }
                
                if (match) return EvaluationResult(v);
            }
            
            return EvaluationError {
                "Expected value to be of type " + toString(getType()) +
                ", but found " + toString(actual) + " instead."
            };
        },
        [&](const EvaluationError& err) -> EvaluationResult { return err; }
    );
}

EvaluationResult ToString::evaluate(const EvaluationParameters& params) const {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&] (const Value& v) -> EvaluationResult {
            return v.match(
                [&] (const std::string& s) -> EvaluationResult { return s; },
                [&] (float) -> EvaluationResult { return stringify(v); },
                [&] (bool) -> EvaluationResult { return stringify(v); },
                [&] (const NullValue&) -> EvaluationResult { return stringify(v); },
                [&] (const auto& v) -> EvaluationResult {
                    return EvaluationError {
                        "Expected a primitive value in [\"string\", ...], but found " + toString(typeOf(v)) + " instead."
                    };
                }
            );
        },
        [&] (const EvaluationError& err) -> EvaluationResult { return err; }
    );
}

EvaluationResult ToNumber::evaluate(const EvaluationParameters& params) const {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&] (const Value& v) -> EvaluationResult {
            if (v.is<float>()) { return v.get<float>(); }
            if (v.is<std::string>()) {
                const std::string& s = v.get<std::string>();
                try {
                    return std::stof(s);
                } catch(std::exception) {
                }
            }
            return EvaluationError {
                "Could not convert " + stringify(v) + " to number."
            };
        },
        [&] (const EvaluationError& err) -> EvaluationResult { return err; }
    );
}

EvaluationResult ToBoolean::evaluate(const EvaluationParameters& params) const {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&] (const Value& v) -> EvaluationResult {
            return v.match(
                [&] (float f) { return (bool)f; },
                [&] (const std::string& s) { return s.length() > 0; },
                [&] (bool b) { return b; },
                [&] (const NullValue&) { return false; },
                [&] (const auto&) { return true; }
            );
        },
        [&] (const EvaluationError& err) -> EvaluationResult { return err; }
    );
}

EvaluationResult ToRGBA::evaluate(const EvaluationParameters& params) const {
    return evaluateFromArgs<mbgl::Color>(params, args[0], [&] (const mbgl::Color& color) {
        return std::vector<Value> { color.r, color.g, color.b, color.a };
    });
}

EvaluationResult ParseColor::evaluate(const EvaluationParameters& params) const {
    return evaluateFromArgs<std::string>(params, args[0], [&] (const std::string& colorString) -> EvaluationResult {
        const auto& result = mbgl::Color::parse(colorString);
        if (result) return EvaluationResult(*result);
        return EvaluationError {
            "Could not parse color from value '" + colorString + "'"
        };
    });
}

EvaluationResult RGB::evaluate(const EvaluationParameters& params) const {
    return evaluateFromArgs<float, float, float>(params, args[0], args[1], args[2],
        [&] (float r, float g, float b) -> EvaluationResult {
            return mbgl::Color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        }
    );
}

EvaluationResult RGBA::evaluate(const EvaluationParameters& params) const {
    return evaluateFromArgs<float, float, float, float>(params, args[0], args[1], args[2], args[3],
        [&] (float r, float g, float b, float a) -> EvaluationResult {
            return mbgl::Color(r / 255.0f, g / 255.0f, b / 255.0f, a);
        }
    );
}

bool Get::isFeatureConstant() const {
    return args.size() == 1 ? false : LambdaExpression::isFeatureConstant();
}
EvaluationResult Get::evaluate(const EvaluationParameters& params) const {
    if (args.size() == 1) {
        return evaluateFromArgs<std::string>(params, args[0], [&] (const std::string& key) -> EvaluationResult {
            const auto& value = params.feature.getValue(key);
            if (!value) return EvaluationError { "Property '" + key + "' not found in feature.properties" };
            return convertValue(*value);
        });
    } else {
        return evaluateFromArgs<std::string, std::unordered_map<std::string, Value>>(
            params,
            args[0],
            args[1],
            [&] (const std::string& key, const std::unordered_map<std::string, Value>& object) -> EvaluationResult {
                if (object.find(key) == object.end()) return EvaluationError { "Property '" + key + "' not found in object" };
                return object.at(key);
            }
        );
    }
}

bool Has::isFeatureConstant() const {
    return args.size() == 1 ? false : LambdaExpression::isFeatureConstant();
}
EvaluationResult Has::evaluate(const EvaluationParameters& params) const {
    if (args.size() == 1) {
        return evaluateFromArgs<std::string>(params, args[0], [&] (const std::string& key) -> EvaluationResult {
            const auto& value = params.feature.getValue(key);
            return value ? true : false;
        });
    } else {
        return evaluateFromArgs<std::string, std::unordered_map<std::string, Value>>(
            params,
            args[0],
            args[1],
            [&] (const std::string& key, const std::unordered_map<std::string, Value>& object) -> EvaluationResult {
                return object.find(key) != object.end();
            }
        );
    }
}

EvaluationResult At::evaluate(const EvaluationParameters& params) const {
    return evaluateFromArgs<float, std::vector<Value>>(
        params,
        args[0],
        args[1],
        [&] (float index, const std::vector<Value>& arr) -> EvaluationResult {
            const size_t i = index;
            if (index > arr.size()) {
                return EvaluationError {
                    "Array index out of bounds: " + std::to_string(i) + " >= " + std::to_string(arr.size())
                };
            }
            return arr[i];
        }
    );
}

EvaluationResult Length::evaluate(const EvaluationParameters& params) const {
    const auto& value = args[0]->evaluate(params);
    return value.match(
        [&] (const Value& v) -> EvaluationResult {
            return (float) v.match(
                [&] (const std::string& s) { return s.size(); },
                [&] (const std::vector<Value>& v) { return v.size(); },
                [&] (const auto&) { assert(false); return -1; }
                
            );
        },
        [&] (const EvaluationError& error) -> EvaluationResult { return error; }
    );
}

bool Properties::isFeatureConstant() const { return false; }
EvaluationResult Properties::evaluate(const EvaluationParameters& params) const {
    return convertValue(params.feature.getProperties());
}

bool Id::isFeatureConstant() const { return false; }
EvaluationResult Id::evaluate(const EvaluationParameters& params) const {
    const auto& id = params.feature.getID();
    if (!id) return EvaluationError { "Property 'id' not found in feature" };
    return id->match(
        [&](const std::string& s) -> EvaluationResult { return s; },
        [&](const auto& n) -> EvaluationResult { return *numericValue<float>(n); }
    );
}

bool GeometryType::isFeatureConstant() const { return false; }
EvaluationResult GeometryType::evaluate(const EvaluationParameters& params) const {
    switch(params.feature.getType()) {
        case FeatureType::Unknown: return std::string("Unknown");
        case FeatureType::LineString: return std::string("LineString");
        case FeatureType::Point: return std::string("Point");
        case FeatureType::Polygon: return std::string("Polygon");
    }
}

EvaluationResult Plus::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo + next; });
}

EvaluationResult Times::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo * next; });
}

EvaluationResult Minus::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo - next; });
}

EvaluationResult Divide::evaluate(const EvaluationParameters& params) const  {
    return evaluateBinaryOperator<float>(params, args,
        {}, [](float memo, float next) { return memo / next; });
}

} // namespace expression
} // namespace style
} // namespace mbgl
