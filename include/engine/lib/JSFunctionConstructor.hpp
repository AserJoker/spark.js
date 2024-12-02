#pragma once
#include "engine/base/JSValueType.hpp"
namespace spark::engine {
class JSFunctionConstructor {
private:
  static JS_FUNC(toPrimitive);
  static JS_FUNC(toString);
  static JS_FUNC(name);
  static JS_FUNC(call);

public:
  static JS_FUNC(constructor);
  static void initialize(common::AutoPtr<JSContext> ctx,
                         common::AutoPtr<JSValue> Function);
};
} // namespace spark::engine