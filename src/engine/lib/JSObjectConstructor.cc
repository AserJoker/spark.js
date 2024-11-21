#include "engine/lib/JSObjectConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
#include "error/JSTypeError.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSObjectConstructor::valueOf) {
  if (self->getType() == JSValueType::JS_NULL) {
    throw error::JSTypeError(L"Cannot convert undefined or null to object");
  }
  if (self->getType() == JSValueType::JS_UNDEFINED) {
    throw error::JSTypeError(L"Cannot convert undefined or null to object");
  }
  if (self->getType() < JSValueType::JS_OBJECT) {
    return self->pack(ctx);
  }
  return self;
}

JS_FUNC(JSObjectConstructor::toString) {
  if (self->getType() == JSValueType::JS_NULL) {
    return ctx->createString(L"[object Null]");
  }
  if (self->getType() == JSValueType::JS_UNDEFINED) {
    return ctx->createString(L"[object Undefined]");
  }
  if (self->getType() < JSValueType::JS_OBJECT) {
    return JSObjectConstructor::toString(ctx, self->pack(ctx), args);
  }
  std::wstring tag = L"Object";
  auto toStringTag =
      self->getProperty(ctx, ctx->Symbol()->getProperty(ctx, L"toStringTag"));
  if (toStringTag->getType() == JSValueType::JS_STRING) {
    tag = toStringTag->getString().value();
  }
  return ctx->createString(fmt::format(L"[object {}]", tag));
}

JS_FUNC(JSObjectConstructor::constructor) {
  if (!args.empty() && !args[0]->isNull() && !args[0]->isUndefined()) {
    return args[0]->pack(ctx);
  } else {
    auto res = self;
    if (res->isNull() || res->isUndefined()) {
      res = ctx->createObject();
    }
    return res;
  }
}

void JSObjectConstructor::initialize(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> Object) {
  auto proto = Object->getProperty(ctx, L"prototype");
  proto->setProperty(
      ctx, L"toString",
      ctx->createFunction(&JSObjectConstructor::toString, L"toString"));

  proto->setProperty(
      ctx, L"valueOf",
      ctx->createFunction(&JSObjectConstructor::valueOf, L"valueOf"));

  proto->setProperty(ctx, L"constructor", Object);
}