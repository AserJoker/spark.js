#pragma once
#include "common/Array.hpp"
#include "common/AutoPtr.hpp"
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
  JS_OBJECT,
  JS_NATIVE_FUNCTION,
  JS_FUNCTION,
  JS_EXCEPTION,
  JS_CLASS,
  JS_SYMBOL
};
class JSValue;
class JSContext;
using JSFunction = common::AutoPtr<JSValue>(
    common::AutoPtr<JSContext>, common::AutoPtr<JSValue>,
    common::Array<common::AutoPtr<JSValue>>);
} // namespace spark::engine