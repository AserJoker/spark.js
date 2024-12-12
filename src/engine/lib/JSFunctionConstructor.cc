#include "engine/lib/JSFunctionConstructor.hpp"
#include "common/AutoPtr.hpp"
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

JS_FUNC(JSFunctionConstructor::apply) {
  auto arr = ctx->createArray();
  for (size_t index = 1; index < args.size(); index++) {
    arr->setIndex(ctx, (uint32_t)(index - 1), args[index]);
  }
  return self->apply(ctx, args[0], {arr});
}

JS_FUNC(JSFunctionConstructor::bind) {
  auto bind = ctx->undefined();
  if (args.size()) {
    bind = args[0];
  }
  if (self->isFunction()) {
    if (self->getType() == JSValueType::JS_NATIVE_FUNCTION) {
      auto e = self->getEntity<JSNativeFunctionEntity>();
      auto result = ctx->createNativeFunction(e->getCallee(), e->getClosure(),
                                              e->getFunctionName());
      auto e2 = result->getEntity<JSNativeFunctionEntity>();
      if (e->getBind() != nullptr) {
        bind = self->getBind(ctx);
      }
      result->setBind(ctx, bind);
      return result;
    } else if (self->getType() == JSValueType::JS_FUNCTION) {
      auto e = self->getEntity<JSFunctionEntity>();
      common::AutoPtr<JSValue> result;
      if (e->isGenerator()) {
        result = ctx->createGenerator(e->getModule());
      } else {
        result = ctx->createFunction(e->getModule());
      }
      auto e2 = result->getEntity<JSFunctionEntity>();
      e2->setAddress(e->getAddress());
      e2->setAsync(e->getAsync());
      e2->setFuncName(e->getFuncName());
      e2->setLength(e->getLength());
      e2->setSource(L"function anonymouse(){ [native code] }");
      for (auto &[k, v] : e->getClosure()) {
        e2->setClosure(k, v);
        result->getStore()->appendChild(v);
      }
      if (e->getBind() != nullptr) {
        bind = self->getBind(ctx);
      }
      result->setBind(ctx, bind);
      return result;
    }
  }
  throw error::JSTypeError(L"Bind must be called on a function");
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

  prototype->setPropertyDescriptor(ctx, L"call",
                                   ctx->createNativeFunction(call, L"call"));

  prototype->setPropertyDescriptor(ctx, L"apply",
                                   ctx->createNativeFunction(apply, L"apply"));

  prototype->setPropertyDescriptor(ctx, L"bind",
                                   ctx->createNativeFunction(bind, L"bind"));
}