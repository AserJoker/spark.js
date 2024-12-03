#include "engine/lib/JSIteratorConstructor.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSIteratorConstructor::constructor) { return self; }

JS_FUNC(JSIteratorConstructor::iterator) { return self; }

common::AutoPtr<JSValue>
JSIteratorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype = ctx->createObject();

  auto Iterator = ctx->createNativeFunction(constructor);

  prototype->setProperty(ctx, L"constructor", Iterator);

  Iterator->setProperty(ctx, L"prototype", prototype);

  prototype->setProperty(
      ctx, ctx->Symbol()->getProperty(ctx, L"Symbol.iterator"),
      ctx->createNativeFunction(iterator, L"[Symbol.iterator]"));
  
  return Iterator;
}