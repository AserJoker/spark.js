#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>
#include <unordered_map>
namespace spark::engine {
class JSSymbolConstructor {
private:
  struct JSSymbolOpaque : public common::Object {
    std::unordered_map<std::wstring, JSEntity *> symbols;
  };

private:
  static JS_FUNC(toPrimitive);
  static JS_FUNC(toString);
  static JS_FUNC(valueOf);
  static JS_FUNC(_for);
  static JS_FUNC(forKey);
  static JS_FUNC(description);

public:
  static JS_FUNC(constructor);
  static void initialize(common::AutoPtr<JSContext> ctx,
                         common::AutoPtr<JSValue> Symbol,
                         common::AutoPtr<JSValue> prototype);
};
} // namespace spark::engine