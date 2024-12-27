#pragma once
#include "engine/base/JSValueType.hpp"
namespace spark::engine {
class JSFunctionConstructor {
private:
  static JS_FUNC(toPrimitive);
  static JS_FUNC(toString);
  static JS_FUNC(call);
  static JS_FUNC(apply);
  static JS_FUNC(bind);

public:
  static JS_FUNC(constructor);
  static void initialize(common::AutoPtr<JSContext> ctx,
                         common::AutoPtr<JSValue> Function,
                         common::AutoPtr<JSValue> prototype);
};
} // namespace spark::engine