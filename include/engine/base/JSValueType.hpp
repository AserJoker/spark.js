#pragma once
#include "common/AutoPtr.hpp"
#include <vector>

namespace spark::engine {
enum class JSValueType {
  JS_UNINITIALIZED = 0,
  JS_INTERNAL,
  JS_UNDEFINED,
  JS_NULL,
  JS_NUMBER,
  JS_BIGINT,
  JS_NAN,
  JS_INFINITY,
  JS_STRING,
  JS_BOOLEAN,
  JS_SYMBOL,
  JS_OBJECT,
  JS_FUNCTION,
  JS_EXCEPTION,
  JS_ARRAY,
  JS_CLASS
};
class JSValue;
class JSContext;
using JSFunction = common::AutoPtr<JSValue>(
    common::AutoPtr<JSContext>, common::AutoPtr<JSValue>,
    std::vector<common::AutoPtr<JSValue>>);

#define JS_FUNCTION(funcname)                                                  \
  common::AutoPtr<JSValue> funcname(                                           \
      common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> self,           \
      std::vector<common::AutoPtr<JSValue>> args)
} // namespace spark::engine