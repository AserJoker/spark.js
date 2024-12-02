#include "engine/lib/JSFunctionConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "error/JSTypeError.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSFunctionConstructor::constructor) { return self; }

JS_FUNC(JSFunctionConstructor::toString) {
  if (self->getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto entity = self->getEntity<JSNativeFunctionEntity>();

    return ctx->createString(fmt::format(L"function {}(){{ [native code] }}",
                                         entity->getFunctionName()));
  }
  if (self->getType() == JSValueType::JS_FUNCTION) {
    auto entity = self->getEntity<JSFunctionEntity>();
    return ctx->createString(fmt::format(L"{}", entity->getFunctionSource()));
  }

  throw error::JSTypeError(
      L"Function.prototype.toString called on incompatible object");
}

JS_FUNC(JSFunctionConstructor::name) {
  if (self->getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto entity = self->getEntity<JSNativeFunctionEntity>();
    return ctx->createString(entity->getFunctionName());
  }
  if (self->getType() == JSValueType::JS_FUNCTION) {
    auto entity = self->getEntity<JSFunctionEntity>();
    return ctx->createString(entity->getFuncName());
  }
  throw error::JSTypeError(
      L"Function.prototype.toString called on incompatible object");
}

void JSFunctionConstructor::initialize(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> Function) {
  auto proto = Function->getProperty(ctx, L"prototype");
  proto->setProperty(ctx, L"toString",
                     ctx->createNativeFunction(toString, L"toString"));
  proto->setProperty(ctx, L"constructor", Function);

  proto->setProperty(ctx, ctx->Symbol()->getProperty(ctx, L"toStringTag"),
                     ctx->createString(L"Function"));

  proto->setPropertyDescriptor(
      ctx, L"name",
      {
          .configurable = true,
          .enumable = false,
          .value = nullptr,
          .writable = false,
          .get = ctx->createNativeFunction(name)->getEntity(),
          .set = nullptr,
      });
}