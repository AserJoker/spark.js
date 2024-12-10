#include "engine/lib/JSFunctionConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "error/JSTypeError.hpp"
#include <fmt/xchar.h>
#include <vector>
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
JS_FUNC(JSFunctionConstructor::call) {
  return self->apply(ctx, args[0], std::vector(args.begin() + 1, args.end()));
}

void JSFunctionConstructor::initialize(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> Function,
                                       common::AutoPtr<JSValue> prototype) {
  prototype->setPropertyDescriptor(
      ctx, L"toString", ctx->createNativeFunction(toString, L"toString"));

  prototype->setPropertyDescriptor(
      ctx, ctx->Symbol()->getProperty(ctx, L"toStringTag"),
      ctx->createString(L"Function"));

  prototype->setPropertyDescriptor(ctx, L"name",
                                   ctx->createNativeFunction(name), nullptr);
}