#pragma once
#include "engine/runtime/JSContext.hpp"
namespace spark::engine {
class JSRegexConstructor {
private:
  static JS_FUNC(test);
  static JS_FUNC(exec);

public:
  static JS_FUNC(constructor);
  static common::AutoPtr<JSValue> initialize(common::AutoPtr<JSContext> ctx);
};
}; // namespace spark::engine