#pragma once
#include "engine/runtime/JSContext.hpp"
namespace spark::engine {
class JSRegexConstructor {
private:
  static JS_FUNC(test);
  static JS_FUNC(exec);
  static JS_FUNC(getDotAll);
  static JS_FUNC(getGlobal);
  static JS_FUNC(getMultiline);
  static JS_FUNC(getUnicode);
  static JS_FUNC(getSticky);
  static JS_FUNC(getIgnoreCase);
  static JS_FUNC(getFlags);
  static JS_FUNC(getSource);
  static JS_FUNC(getLastIndex);

public:
  static JS_FUNC(constructor);
  static common::AutoPtr<JSValue> initialize(common::AutoPtr<JSContext> ctx);
};
}; // namespace spark::engine