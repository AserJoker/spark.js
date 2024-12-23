#pragma once
#include "engine/base/JSValueType.hpp"
namespace spark::engine {
class JSAsyncFunctionConstructor {

public:
  static JS_FUNC(constructor);
  static common::AutoPtr<JSValue> initialize(common::AutoPtr<JSContext> ctx);
};
} // namespace spark::engine