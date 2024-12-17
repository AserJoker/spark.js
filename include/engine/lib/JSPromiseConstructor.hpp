#pragma once
#include "engine/runtime/JSContext.hpp"
namespace spark::engine {
class JSPromiseConstructor {
private:
  static JS_FUNC(then);
  static JS_FUNC(catch_);
  static JS_FUNC(finally);
  static JS_FUNC(resolve);
  static JS_FUNC(reject);

public:
  static JS_FUNC(constructor);
  static common::AutoPtr<JSValue> initialize(common::AutoPtr<JSContext> ctx);
};
} // namespace spark::engine