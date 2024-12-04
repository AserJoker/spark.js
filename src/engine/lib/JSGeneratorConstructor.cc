#include "engine/lib/JSGeneratorConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSCoroutineContext.hpp"
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSGeneratorConstructor::next) {
  auto result = ctx->createObject();
  auto co = self->getOpaque<vm::JSCoroutineContext>();
  auto vm = ctx->getRuntime()->getVirtualMachine();
  auto eval = vm->getContext();
  auto scope = ctx->getScope();
  ctx->pushCallStack({.funcname = co.funcname});
  vm->setContext(co.eval);
  ctx->setScope(co.scope);
  vm->run(ctx, co.module, co.pc);
  auto pc = *co.eval->stack.rbegin();
  co.eval->stack.pop_back();
  auto value = *co.eval->stack.rbegin();
  co.eval->stack.pop_back();
  result->setProperty(ctx, L"value", value);
  co.pc = pc->getNumber().value();
  ctx->popCallStack();
  ctx->setScope(scope);
  self->setOpaque(co);
  vm->setContext(eval);
  result->setProperty(ctx, L"done", ctx->falsely());
  return result;
}

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
