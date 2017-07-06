#include <mbgl/style/function/type.hpp>

namespace mbgl {
namespace style {
namespace expression {
namespace type {

Primitive Primitive::Null = {"Null"};
Primitive Primitive::String = {"String"};
Primitive Primitive::Number = {"Number"};
Primitive Primitive::Color = {"Color"};
Primitive Primitive::Object = {"Object"};

// Need to add Array(Types::Value) to the member list somehow...
Type Primitive::Value = Variant({
    Primitive::Null,
    Primitive::String,
    Primitive::Number,
    Primitive::Color,
    Primitive::Object
});

} // namespace type
} // namespace expression
} // namespace style
} // namespace mbgl
