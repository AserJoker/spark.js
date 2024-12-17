#include "engine/lib/JSTypeErrorConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>

using namespace spark;
using namespace spark::engine;
JS_FUNC(JSTypeErrorConstructor::constructor) {
  return ctx->Error()->apply(ctx, self, args);
}

common::AutoPtr<JSValue>
JSTypeErrorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Error()->getProperty(ctx, L"prototype"));
  auto Error =
      ctx->createNativeFunction(&constructor, L"TypeError", L"TypeError");
  Error->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", prototype);
  return Error;
}