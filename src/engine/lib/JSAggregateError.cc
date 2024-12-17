#include "engine/base/JSValueType.hpp"
#include "engine/lib/JSAggregateErrorConstructor.hpp"
#include <fmt/xchar.h>

using namespace spark;
using namespace spark::engine;
JS_FUNC(JSAggregateErrorConstructor::constructor) {
  if (self->getType() != JSValueType::JS_OBJECT) {
    auto prop = ctx->AggregateError()->getProperty(ctx, L"prototype");
    self = ctx->createObject(prop);
  }
  return ctx->Error()->apply(ctx, self, args);
}

common::AutoPtr<JSValue>
JSAggregateErrorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Error()->getProperty(ctx, L"prototype"));
  auto Error = ctx->createNativeFunction(&constructor, L"AggregateError",
                                         L"AggregateError");
  Error->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", prototype);
  return Error;
}