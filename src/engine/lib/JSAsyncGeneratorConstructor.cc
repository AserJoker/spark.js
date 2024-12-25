#include "engine/lib/JSAsyncGeneratorConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSTasKEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSAsmOperator.hpp"
#include "vm/JSCoroutineContext.hpp"
#include <cstdint>
#include <string>
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSAsyncGeneratorConstructor::next) {
  auto &co = self->getOpaque<vm::JSCoroutineContext>();
  if (co.done) {
    return co.value;
  }
  auto resolver = [](common::AutoPtr<JSContext> ctx,
                     common::AutoPtr<JSValue> self,
                     std::vector<common::AutoPtr<JSValue>> args)
      -> common::AutoPtr<JSValue> {
    auto last = ctx->load(L"#promise");
    auto arg = ctx->load(L"#arg");
    auto resolve = args[0];
    auto reject = args[1];
    auto callback = [](common::AutoPtr<JSContext> ctx,
                       common::AutoPtr<JSValue> self,
                       std::vector<common::AutoPtr<JSValue>> args)
        -> common::AutoPtr<JSValue> {
      auto arg = ctx->load(L"#arg");
      auto resolve = ctx->load(L"#resolve");
      auto reject = ctx->load(L"#reject");
      auto &co = self->getOpaque<vm::JSCoroutineContext>();
      auto vm = ctx->getRuntime()->getVirtualMachine();
      auto eval = vm->getContext();
      auto scope = ctx->getScope();
      ctx->pushCallStack({.funcname = co.funcname});
      vm->setContext(co.eval);
      ctx->setScope(co.scope);
      co.eval->stack.push_back(co.scope->createValue(arg->getStore()));
      common::AutoPtr<JSValue> result;
      vm->run(ctx, co.module, co.pc);
      co.scope = ctx->getScope();
      auto task = *co.eval->stack.rbegin();
      co.eval->stack.pop_back();
      auto callback = [](common::AutoPtr<JSContext> ctx,
                         common::AutoPtr<JSValue> self,
                         std::vector<common::AutoPtr<JSValue>> args)
          -> common::AutoPtr<JSValue> {
        auto resolve = ctx->load(L"resolve");
        auto done = ctx->load(L"done");
        auto obj = ctx->createObject();
        auto value = ctx->undefined();
        if (!args.empty()) {
          value = args[0];
        }
        obj->setProperty(ctx, L"done", done);
        obj->setProperty(ctx, L"value", value);
        resolve->apply(ctx, ctx->undefined(), {obj});
        return ctx->undefined();
      };
      auto awaitCallback = [](common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> self,
                              std::vector<common::AutoPtr<JSValue>> args)
          -> common::AutoPtr<JSValue> {
        auto resolve = ctx->load(L"resolve");
        resolve->apply(ctx, ctx->undefined(), {args[0]});
        return ctx->undefined();
      };
      common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
      closure[L"resolve"] = resolve;
      closure[L"done"] =
          ctx->createBoolean(task->getType() != JSValueType::JS_TASK);
      auto callbackFunc = ctx->createNativeFunction(callback, closure);
      auto awaitCallbackFunc =
          ctx->createNativeFunction(awaitCallback, closure);
      if (task->getType() == JSValueType::JS_TASK) {
        auto e = task->getEntity<JSTaskEntity>();
        auto currentOpt =
            (vm::JSAsmOperator) *
            (uint16_t *)(&co.module->codes[e->getAddress() - sizeof(uint16_t)]);
        if (currentOpt == vm::JSAsmOperator::AWAIT ||
            currentOpt == vm::JSAsmOperator::AWAIT_NEXT) {
          co.pc = e->getAddress();
          co.value = ctx->Promise()
                         ->getProperty(ctx, L"resolve")
                         ->apply(ctx, ctx->Promise());
          ctx->popCallStack();
          ctx->setScope(scope);
          vm->setContext(eval);
          self->getProperty(ctx, L"next")
              ->apply(ctx, self, {ctx->createValue(e->getValue())});
          co.value->getProperty(ctx, L"then")
              ->apply(ctx, co.value, {awaitCallbackFunc, reject});
          return ctx->undefined();
        } else {
          auto next = ctx->Promise()
                          ->getProperty(ctx, L"resolve")
                          ->apply(ctx, ctx->Promise(),
                                  {ctx->createValue(e->getValue())});
          next->getProperty(ctx, L"then")
              ->apply(ctx, next, {callbackFunc, reject});
          co.pc = e->getAddress();
        }
      } else if (task->getType() == JSValueType::JS_EXCEPTION) {
        reject->apply(ctx, ctx->undefined(), {ctx->createError(task)});
        co.done = true;
        co.pc = co.module->codes.size();
      } else {
        auto next = ctx->Promise()
                        ->getProperty(ctx, L"resolve")
                        ->apply(ctx, ctx->Promise(), {task});
        co.done = true;
        next->getProperty(ctx, L"then")
            ->apply(ctx, next, {callbackFunc, reject});
        co.pc = co.module->codes.size();
      }
      ctx->popCallStack();
      ctx->setScope(scope);
      vm->setContext(eval);
      return ctx->undefined();
    };
    common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
    closure[L"#arg"] = arg;
    closure[L"#resolve"] = resolve;
    closure[L"#reject"] = reject;
    auto callbackFunc = ctx->createNativeFunction(callback, closure);
    callbackFunc->setBind(ctx, self);
    auto asyncLast = ctx->Promise()
                         ->getProperty(ctx, L"resolve")
                         ->apply(ctx, ctx->Promise(), {last});
    asyncLast->getProperty(ctx, L"then")->apply(ctx, asyncLast, {callbackFunc});
    return ctx->undefined();
  };
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  if (co.value != nullptr) {
    closure[L"#promise"] = co.value;
  } else {
    closure[L"#promise"] = ctx->undefined();
  }
  if (args.empty()) {
    closure[L"#arg"] = ctx->undefined();
  } else {
    closure[L"#arg"] = args[0];
  }
  auto resolverFunc = ctx->createNativeFunction(resolver, closure);
  resolverFunc->setBind(ctx, self);
  auto promise = ctx->constructObject(ctx->Promise(), {resolverFunc});
  co.value = promise;
  self->getStore()->appendChild(promise->getStore());
  return promise;
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
      ctx->createObject(ctx->AsyncIterator()->getProperty(ctx, L"prototype"));
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
