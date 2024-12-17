#include "engine/lib/JSRangeErrorConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>

using namespace spark;
using namespace spark::engine;
JS_FUNC(JSRangeErrorConstructor::constructor) {
  if (self->getType() != JSValueType::JS_OBJECT) {
    auto prop = ctx->RangeError()->getProperty(ctx, L"prototype");
    self = ctx->createObject(prop);
  }
  return ctx->Error()->apply(ctx, self, args);
}

common::AutoPtr<JSValue>
JSRangeErrorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Error()->getProperty(ctx, L"prototype"));
  auto Error =
      ctx->createNativeFunction(&constructor, L"RangeError", L"RangeError");
  Error->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", prototype);
  return Error;
}