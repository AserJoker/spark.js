#include "engine/lib/JSAsyncFunctionConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSAsyncFunctionConstructor::constructor) { return self; }

common::AutoPtr<JSValue>
JSAsyncFunctionConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Function()->getProperty(ctx, L"prototype"));
  auto AsyncFunction = ctx->createNativeFunction(constructor, L"AsyncFunction");
  AsyncFunction->setProperty(ctx, L"prototype", prototype);
  prototype->setProperty(ctx, L"constructor", AsyncFunction);

  prototype->setPropertyDescriptor(
      ctx, ctx->Symbol()->getProperty(ctx, L"toStringTag"),
      ctx->createString(L"AsyncFunction"));
  return AsyncFunction;
}