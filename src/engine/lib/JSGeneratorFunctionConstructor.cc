#include "engine/lib/JSGeneratorFunctionConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"

using namespace spark;
using namespace spark::engine;
JS_FUNC(JSGeneratorFunctionConstructor::constructor) { return self; }
common::AutoPtr<JSValue>
JSGeneratorFunctionConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype = ctx->createObject();
  auto GeneratorFunction =
      ctx->createNativeFunction(constructor, L"GeneratorFunction");
  GeneratorFunction->setProperty(ctx, L"prototype", prototype);
  prototype->setProperty(ctx, L"constructor", GeneratorFunction);
  return GeneratorFunction;
}