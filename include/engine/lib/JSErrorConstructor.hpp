#pragma once
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
namespace spark::engine {
class JSErrorConstructor {
private:
  static JS_FUNC(toString);

public:
  static JS_FUNC(constructor);
  static void initialize(common::AutoPtr<JSContext> ctx,
                         common::AutoPtr<JSValue> Error,
                         common::AutoPtr<JSValue> prototype);
};
} // namespace spark::engine