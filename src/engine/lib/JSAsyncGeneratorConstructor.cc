#include "engine/lib/JSAsyncGeneratorConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSTasKEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSCoroutineContext.hpp"
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSAsyncGeneratorConstructor::next) {
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
    co.eval->stack.push_back(co.scope->createValue(args[0]->getStore()));
  }
  vm->run(ctx, co.module, co.pc);
  co.scope = ctx->getScope();
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
    co.value->getStore()->appendChild(ctx->undefined()->getStore());
    co.pc = co.module->codes.size();
  } else {
    result->setProperty(ctx, L"done", ctx->truly());
    result->setProperty(ctx, L"value", task);
    co.value = task;
    co.value->getStore()->appendChild(task->getStore());
    co.pc = co.module->codes.size();
  }
  ctx->popCallStack();
  ctx->setScope(scope);
  vm->setContext(eval);
  return result;
}

JS_FUNC(JSAsyncGeneratorConstructor::throw_) {
  auto arg = ctx->undefined();
  if (!args.empty()) {
    arg = args[0];
  }
  arg = ctx->createException(arg);
  auto &co = self->getOpaque<vm::JSCoroutineContext>();
  co.pc = co.module->codes.size();
  return next(ctx, self, {arg});
}
JS_FUNC(JSAsyncGeneratorConstructor::return_) {
  auto arg = ctx->undefined();
  if (!args.empty()) {
    arg = args[0];
  }
  auto &co = self->getOpaque<vm::JSCoroutineContext>();
  co.pc = co.module->codes.size();
  return next(ctx, self, {arg});
}

JS_FUNC(JSAsyncGeneratorConstructor::constructor) { return self; }

common::AutoPtr<JSValue>
JSAsyncGeneratorConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype =
      ctx->createObject(ctx->Iterator()->getProperty(ctx, L"prototype"));
  auto Generator = ctx->createNativeFunction(constructor, L"Generator");
  Generator->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"constructor", Generator);
  prototype->setPropertyDescriptor(ctx, L"next",
                                   ctx->createNativeFunction(next, L"next"));
  prototype->setPropertyDescriptor(ctx, L"throw",
                                   ctx->createNativeFunction(throw_, L"throw"));
  prototype->setPropertyDescriptor(
      ctx, L"return", ctx->createNativeFunction(return_, L"return"));
  return Generator;
}
