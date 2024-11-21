#pragma once
#include "engine/base/JSValueType.hpp"
namespace spark::engine {
class JSObjectConstructor {
private:
  static JS_FUNC(toPrimitive);
  static JS_FUNC(toString);
  static JS_FUNC(valueOf);

public:
  static JS_FUNC(constructor);
  static void initialize(common::AutoPtr<JSContext> ctx,
                         common::AutoPtr<JSValue> Object);
};
} // namespace spark::engine