#include "engine/lib/JSPromiseConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSPromiseEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSTypeError.hpp"
#include <string>
using namespace spark;
using namespace spark::engine;

static JS_FUNC(resolve) { return ctx->undefined(); }
static JS_FUNC(reject) { return ctx->undefined(); }
static JS_FUNC(onSettled) {
  auto next = ctx->load(L"next");
  auto onFulfilled = ctx->load(L"onFulfilled");
  auto entity = self->getEntity<JSPromiseEntity>();
  auto value = ctx->createValue(entity->getValue());
  auto res = onFulfilled->apply(ctx, ctx->undefined(), {value});
  if (res->getType() == spark::engine::JSValueType::JS_EXCEPTION) {
    auto e = res->getEntity<JSExceptionEntity>();
    if (e->getTarget() != nullptr) {
      auto error = ctx->createValue(e->getTarget());
    } else {
    }
  }
  return ctx->undefined();
}
static JS_FUNC(onError) { return ctx->undefined(); }
static JS_FUNC(onDefer) { return ctx->undefined(); }

JS_FUNC(JSPromiseConstructor::resolve) { return ctx->undefined(); }
JS_FUNC(JSPromiseConstructor::reject) { return ctx->undefined(); }

JS_FUNC(JSPromiseConstructor::constructor) {
  common::AutoPtr<JSValue> resolver;
  if (args.empty()) {
    resolver = ctx->undefined();
  } else {
    resolver = args[0];
  }
  if (!resolver->isFunction()) {
    throw error::JSTypeError(
        fmt::format(L"Promise resolver '{}' is not a function",
                    resolver->toString(ctx)->getString().value()));
  }
  if (self->getEntity<JSPromiseEntity>() == nullptr) {
    throw error::JSTypeError(
        L"Promise constructor cannot be invoked without 'new'");
  }
  auto resolveFunc = ctx->createNativeFunction(resolve);
  auto rejectFunc = ctx->createNativeFunction(reject);
  resolveFunc->setBind(ctx, self);
  rejectFunc->setBind(ctx, self);
  resolver->apply(ctx, ctx->undefined(), {resolveFunc, rejectFunc});
  return ctx->undefined();
}

JS_FUNC(JSPromiseConstructor::then) {
  auto entity = self->getEntity<JSPromiseEntity>();
  common::AutoPtr<JSValue> onFulfilled = ctx->undefined();
  common::AutoPtr<JSValue> onRejected = ctx->undefined();
  if (!args.empty() && args[0]->isFunction()) {
    onFulfilled = args[0];
  }
  if (args.size() > 1 && args[1]->isFunction()) {
    onRejected = args[1];
  }
  auto next = ctx->createPromise();
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"next"] = next;
  if (onFulfilled->isFunction()) {
    closure[L"onFulfilled"] = onFulfilled;
  }
  if (onRejected->isFunction()) {
    closure[L"onRejected"] = onRejected;
  }
  auto onSettledFunc = ctx->createNativeFunction(onSettled, closure);
  onSettledFunc->setBind(ctx, self);
  auto onRejectedFunc = ctx->createNativeFunction(onError, closure);
  onRejectedFunc->setBind(ctx, self);
  if (entity->getStatus() == JSPromiseEntity::Status::FULFILLED) {
    ctx->createMicroTask(onSettledFunc);
  } else if (entity->getStatus() == JSPromiseEntity::Status::REJECTED) {
    if (onRejected->isFunction()) {
      ctx->createMicroTask(onRejectedFunc);
    }
  } else {
    if (onFulfilled->isFunction()) {
      entity->getFulfilledCallbacks().push_back(onSettledFunc->getStore());
      self->getStore()->appendChild(onSettledFunc->getStore());
    }
    if (onRejected->isFunction()) {
      entity->getRejectedCallbacks().push_back(onRejected->getStore());
      self->getStore()->appendChild(onRejected->getStore());
    }
  }
  return next;
}
JS_FUNC(JSPromiseConstructor::catch_) {
  auto entity = self->getEntity<JSPromiseEntity>();
  common::AutoPtr<JSValue> onRejected = ctx->undefined();
  if (!args.empty() && args[0]->isFunction()) {
    onRejected = args[0];
  }
  auto next = ctx->createPromise();
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"next"] = next;
  if (onRejected->isFunction()) {
    closure[L"onRejected"] = onRejected;
  }
  auto onRejectedFunc = ctx->createNativeFunction(onError, closure);
  onRejectedFunc->setBind(ctx, self);
  if (entity->getStatus() == JSPromiseEntity::Status::REJECTED) {
    ctx->createMicroTask(onRejectedFunc);
  } else if (entity->getStatus() == JSPromiseEntity::Status::PENDING) {
    entity->getRejectedCallbacks().push_back(onRejectedFunc->getStore());
    self->getStore()->appendChild(onRejectedFunc->getStore());
  }
  return next;
}
JS_FUNC(JSPromiseConstructor::finally) {
  auto entity = self->getEntity<JSPromiseEntity>();
  common::AutoPtr<JSValue> onFinally = ctx->undefined();
  if (!args.empty() && args[0]->isFunction()) {
    onFinally = args[0];
  }
  auto next = ctx->createPromise();
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"next"] = next;
  if (onFinally->isFunction()) {
    closure[L"onFinally"] = onFinally;
  }
  auto onFinallyFunc = ctx->createNativeFunction(onDefer, closure);
  onFinallyFunc->setBind(ctx, self);
  if (entity->getStatus() != JSPromiseEntity::Status::PENDING) {
    ctx->createMicroTask(onFinallyFunc);
  } else {
    entity->getFinallyCallbacks().push_back(onFinallyFunc->getStore());
    self->getStore()->appendChild(onFinallyFunc->getStore());
  }
  return next;
}

common::AutoPtr<JSValue>
JSPromiseConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto Promise = ctx->createNativeFunction(constructor, L"Promise");
  auto prototype = ctx->createObject();
  prototype->setPropertyDescriptor(ctx, L"constructor", Promise);
  Promise->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"then",
                                   ctx->createNativeFunction(then, L"then"));
  prototype->setPropertyDescriptor(ctx, L"catch",
                                   ctx->createNativeFunction(catch_, L"catch"));
  prototype->setPropertyDescriptor(
      ctx, L"finally", ctx->createNativeFunction(finally, L"finally"));
  return Promise;
}