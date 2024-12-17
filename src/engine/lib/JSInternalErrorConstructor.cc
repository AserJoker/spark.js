#include "engine/lib/JSInternalErrorConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>

using namespace spark;
using namespace spark::engine;
JS_FUNC(JSInternalErrorConstructor::constructor) {
  return ctx->Error()->apply(ctx, self, args);
}

common::AutoPtr<JSValue>
JSInternalErrorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Error()->getProperty(ctx, L"prototype"));
  auto Error = ctx->createNativeFunction(&constructor, L"InternalError",
                                         L"InternalError");
  Error->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", prototype);
  return Error;
}