#include "engine/lib/JSGeneratorConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSGeneratorConstructor::next) { return ctx->undefined(); }

JS_FUNC(JSGeneratorConstructor::constructor) { return self; }

common::AutoPtr<JSValue>
JSGeneratorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Iterator()->getProperty(ctx, L"prototype"));
  auto Generator = ctx->createNativeFunction(constructor);
  Generator->setProperty(ctx, L"prototype", prototype);
  prototype->setProperty(ctx, L"constructor", Generator);
  prototype->setProperty(ctx, L"next",
                         ctx->createNativeFunction(next, L"next"));
  return Generator;
}
