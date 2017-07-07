#pragma once

#include <memory>
#include <mbgl/style/expression/parsing_context.hpp>
#include <mbgl/style/expression/expression.hpp>
#include <mbgl/style/conversion.hpp>

namespace mbgl {
namespace style {
namespace expression {

using namespace mbgl::style;

template <class V>
std::string getJSType(const V& value) {
    using namespace mbgl::style::conversion;
    if (isUndefined(value)) {
        return "undefined";
    }
    if (isArray(value) || isObject(value)) {
        return "object";
    }
    optional<mbgl::Value> v = toValue(value);
    assert(v);
    return v->match(
        [&] (std::string) { return "string"; },
        [&] (bool) { return "boolean"; },
        [&] (auto) { return "number"; }
    );
}

using ParseResult = variant<CompileError, std::unique_ptr<Expression>>;

template <class V>
ParseResult parseExpression(const V& value, const ParsingContext& context)
{
    using namespace mbgl::style::conversion;
    
    if (isArray(value)) {
        if (arrayLength(value) == 0) {
            CompileError error = {
                "Expected an array with at least one element. If you wanted a literal array, use [\"literal\", []].",
                context.key()
            };
            return error;
        }
        
        const optional<std::string>& op = toString(arrayMember(value, 0));
        if (!op) {
            CompileError error = {
                "Expression name must be a string, but found " + getJSType(arrayMember(value, 0)) +
                    " instead. If you wanted a literal array, use [\"literal\", [...]].",
                context.key(0)
            };
            return error;
        }
        
        if (*op == "+") return PlusExpression::parse(value, context);
        if (*op == "-") return MinusExpression::parse(value, context);
        if (*op == "*") return TimesExpression::parse(value, context);
        if (*op == "/") return DivideExpression::parse(value, context);

        
        return CompileError {
            std::string("Unknown expression \"") + *op + "\". If you wanted a literal array, use [\"literal\", [...]].",
            context.key(0)
        };
    }
    
    if (isObject(value)) {
        return CompileError {
            "Bare objects invalid. Use [\"literal\", {...}] instead.",
            context.key()
        };
    }
    
    return LiteralExpression::parse(value, context);
}


} // namespace expression
} // namespace style
} // namespace mbgl
