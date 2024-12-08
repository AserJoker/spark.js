#pragma once
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
namespace spark::engine {
class JSArrayConstructor {
private:
  static JS_FUNC(toString);
  static JS_FUNC(getLength);
  static JS_FUNC(setLength);
  static JS_FUNC(toStringTag);
  static JS_FUNC(join);

public:
  static JS_FUNC(constructor);
  static common::AutoPtr<JSValue> initialize(common::AutoPtr<JSContext> ctx);
};
}; // namespace spark::engine