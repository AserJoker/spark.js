#include "engine/lib/JSGeneratorConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSTasKEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSCoroutineContext.hpp"
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSGeneratorConstructor::next) {
  auto result = ctx->createObject();
  auto &co = self->getOpaque<vm::JSCoroutineContext>();
  if (co.value != nullptr) {
    result->setProperty(ctx, L"value", co.value);
    result->setProperty(ctx, L"done", ctx->truly());
    return result;
  }
  auto vm = ctx->getRuntime()->getVirtualMachine();
  auto eval = vm->getContext();
  auto scope = ctx->getScope();
  ctx->pushCallStack({.funcname = co.funcname});
  vm->setContext(co.eval);
  ctx->setScope(co.scope);
  if (args.empty()) {
    co.eval->stack.push_back(ctx->undefined());
  } else {
    co.eval->stack.push_back(co.scope->createValue(args[0]->getEntity()));
  }
  vm->run(ctx, co.module, co.pc);
  auto task = *co.eval->stack.rbegin();
  co.eval->stack.pop_back();
  if (task->getType() == JSValueType::JS_TASK) {
    auto e = task->getEntity<JSTaskEntity>();
    result->setProperty(ctx, L"done", ctx->falsely());
    result->setProperty(ctx, L"value", ctx->createValue(e->getValue()));
    co.pc = e->getAddress();
  } else if (task->getType() == JSValueType::JS_EXCEPTION) {
    result = task;
    co.value = ctx->undefined();
    co.value->getEntity()->appendChild(ctx->undefined()->getEntity());
    co.pc = co.module->codes.size();
  } else {
    result->setProperty(ctx, L"done", ctx->truly());
    result->setProperty(ctx, L"value", task);
    co.value = task;
    co.value->getEntity()->appendChild(task->getEntity());
    co.pc = co.module->codes.size();
  }
  ctx->popCallStack();
  ctx->setScope(scope);
  vm->setContext(eval);
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
