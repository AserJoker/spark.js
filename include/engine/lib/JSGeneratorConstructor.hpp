#pragma once
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSValue.hpp"
namespace spark::engine {
class JSGeneratorConstructor {
private:
  static JS_FUNC(next);
  static JS_FUNC(return_);
  static JS_FUNC(throw_);

public:
  static JS_FUNC(constructor);
  static common::AutoPtr<JSValue> initialize(common::AutoPtr<JSContext> ctx);
};
} // namespace spark::engine