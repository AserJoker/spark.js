#include "engine/lib/JSAsyncIteratorConstructor.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSAsyncIteratorConstructor::constructor) { return self; }

JS_FUNC(JSAsyncIteratorConstructor::asyncIterator) { return self; }

common::AutoPtr<JSValue>
JSAsyncIteratorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype = ctx->createObject();

  auto Iterator = ctx->createNativeFunction(constructor, L"AsyncIterator");

  prototype->setPropertyDescriptor(ctx, L"constructor", Iterator);

  Iterator->setPropertyDescriptor(ctx, L"prototype", prototype);

  prototype->setPropertyDescriptor(
      ctx, ctx->Symbol()->getProperty(ctx, L"asyncIterator"),
      ctx->createNativeFunction(asyncIterator, L"[Symbol.asyncIterator]"));

  return Iterator;
}