#include "engine/lib/JSSyntaxErrorConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>

using namespace spark;
using namespace spark::engine;
JS_FUNC(JSSyntaxErrorConstructor::constructor) {
  if (self->getType() != JSValueType::JS_OBJECT) {
    auto prop = ctx->SyntaxError()->getProperty(ctx, L"prototype");
    self = ctx->createObject(prop);
  }
  return ctx->Error()->apply(ctx, self, args);
}

common::AutoPtr<JSValue>
JSSyntaxErrorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Error()->getProperty(ctx, L"prototype"));
  auto Error =
      ctx->createNativeFunction(&constructor, L"SyntaxError", L"SyntaxError");
  Error->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", prototype);
  return Error;
}