#include "engine/lib/JSPromiseConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSPromiseEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSTypeError.hpp"
#include <string>
using namespace spark;
using namespace spark::engine;

static JS_FUNC(onSettled) {
  auto entity = self->getEntity<JSPromiseEntity>();
  for (auto &cb : entity->getFinallyCallbacks()) {
    auto func = ctx->createValue(cb);
    auto res = func->apply(ctx, ctx->undefined());
    if (res->isException()) {
      return res;
    }
  }
  auto value = ctx->createValue(entity->getValue());
  for (auto &cb : entity->getFulfilledCallbacks()) {
    auto func = ctx->createValue(cb);
    auto res = func->apply(ctx, ctx->undefined(), {value});
    if (res->isException()) {
      return res;
    }
  }
  return ctx->undefined();
}

static JS_FUNC(onRejected) {
  auto entity = self->getEntity<JSPromiseEntity>();
  for (auto &cb : entity->getFinallyCallbacks()) {
    auto func = ctx->createValue(cb);
    auto res = func->apply(ctx, ctx->undefined());
    if (res->isException()) {
      return res;
    }
  }
  auto value = ctx->createValue(entity->getValue());
  for (auto &cb : entity->getRejectedCallbacks()) {
    auto func = ctx->createValue(cb);
    auto res = func->apply(ctx, ctx->undefined(), {value});
    if (res->isException()) {
      return res;
    }
  }
  return ctx->undefined();
}

static JS_FUNC(resolve) {
  auto entity = self->getEntity<JSPromiseEntity>();
  if (entity->getStatus() == JSPromiseEntity::Status::PENDING) {
    auto value = ctx->undefined();
    if (!args.empty()) {
      value = args[0];
    }
    entity->setValue(value->getStore());
    self->getStore()->appendChild(value->getStore());
    entity->getStatus() = JSPromiseEntity::Status::FULFILLED;
    auto onSettledFunc = ctx->createNativeFunction(onSettled);
    onSettledFunc->setBind(ctx, self);
    ctx->createMicroTask(onSettledFunc);
  }
  return ctx->undefined();
}

static JS_FUNC(reject) {
  auto entity = self->getEntity<JSPromiseEntity>();
  if (entity->getStatus() == JSPromiseEntity::Status::PENDING) {
    auto value = ctx->undefined();
    if (!args.empty()) {
      value = args[0];
    }
    entity->setValue(value->getStore());
    self->getStore()->appendChild(value->getStore());
    entity->getStatus() = JSPromiseEntity::Status::REJECTED;
    auto onRejectedFunc = ctx->createNativeFunction(onRejected);
    onRejectedFunc->setBind(ctx, self);
    ctx->createMicroTask(onRejectedFunc);
  }
  return ctx->undefined();
}

static JS_FUNC(pipeline) {
  auto resolve = ctx->load(L"resolve");
  auto reject = ctx->load(L"reject");
  auto callback = ctx->load(L"callback");
  auto value = ctx->load(L"value");
  auto res = callback->apply(ctx, ctx->undefined(), {value});
  if (res->isException()) {
    reject->apply(ctx, ctx->undefined(), {ctx->createError(res)});
  } else {
    resolve->apply(ctx, ctx->undefined(), {res});
  }
  return ctx->undefined();
}

static JS_FUNC(nextCallback) {
  auto resolve = args[0];
  auto reject = args[1];
  auto callback = ctx->load(L"callback");
  auto value = ctx->load(L"value");
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"resolve"] = resolve;
  closure[L"reject"] = reject;
  closure[L"callback"] = callback;
  closure[L"value"] = value;
  auto pipelineFunc = ctx->createNativeFunction(pipeline, closure);
  ctx->createMicroTask(pipelineFunc);
  return ctx->undefined();
}

static JS_FUNC(onCreateFulfilled) {
  auto value = ctx->load(L"value");
  args[0]->apply(ctx, ctx->undefined(), {value});
  return ctx->undefined();
}

static JS_FUNC(onCreateRejected) {
  auto value = ctx->load(L"value");
  args[1]->apply(ctx, ctx->undefined(), {value});
  return ctx->undefined();
}

static JS_FUNC(onNextResolve) {
  auto value = ctx->undefined();
  if (!args.empty()) {
    value = args[0];
  }
  auto callback = ctx->load(L"callback");
  auto resolve = ctx->load(L"resolve");
  auto reject = ctx->load(L"reject");
  if (callback->isFunction()) {
    common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
    closure[L"resolve"] = resolve;
    closure[L"reject"] = reject;
    closure[L"callback"] = callback;
    closure[L"value"] = value;
    auto res = callback->apply(ctx, ctx->undefined(), {value});
    if (res->isException()) {
      reject->apply(ctx, ctx->undefined(), {ctx->createError(res)});
    } else {
      resolve->apply(ctx, ctx->undefined(), {res});
    }
  } else {
    resolve->apply(ctx, ctx->undefined(), {value});
  }
  return ctx->undefined();
}

static JS_FUNC(onNextReject) {
  auto value = ctx->constructObject(ctx->Error(), {}, {});
  if (!args.empty()) {
    value = args[0];
  }
  auto callback = ctx->load(L"onError");
  auto resolve = ctx->load(L"resolve");
  auto reject = ctx->load(L"reject");
  if (callback->isFunction()) {
    common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
    closure[L"resolve"] = resolve;
    closure[L"reject"] = reject;
    closure[L"callback"] = callback;
    closure[L"value"] = value;
    auto res = callback->apply(ctx, ctx->undefined(), {value});
    if (res->isException()) {
      reject->apply(ctx, ctx->undefined(), {ctx->createError(res)});
    } else {
      resolve->apply(ctx, ctx->undefined(), {res});
    }
  } else {
    reject->apply(ctx, ctx->undefined(), {value});
  }
  return ctx->undefined();
}

static JS_FUNC(nextPendding) {
  auto entity = self->getEntity<JSPromiseEntity>();
  auto callback = ctx->load(L"callback");
  auto onError = ctx->load(L"onError");
  auto resolve = args[0];
  auto reject = args[1];
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"resolve"] = resolve;
  closure[L"reject"] = reject;
  closure[L"callback"] = callback;
  closure[L"onError"] = onError;
  auto onNextResolveFunc = ctx->createNativeFunction(onNextResolve, closure);
  auto onNextRejectFunc = ctx->createNativeFunction(onNextReject, closure);
  entity->getFulfilledCallbacks().push_back(onNextResolveFunc->getStore());
  self->getStore()->appendChild(onNextResolveFunc->getStore());
  entity->getRejectedCallbacks().push_back(onNextRejectFunc->getStore());
  self->getStore()->appendChild(onNextRejectFunc->getStore());
  return ctx->undefined();
}

JS_FUNC(JSPromiseConstructor::resolve) {
  auto value = ctx->undefined();
  if (!args.empty()) {
    value = args[0];
  }
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"value"] = value;
  auto onCreateFulfilledFunc =
      ctx->createNativeFunction(onCreateFulfilled, closure);
  return ctx->constructObject(ctx->Promise(), {onCreateFulfilledFunc}, {});
}

JS_FUNC(JSPromiseConstructor::reject) {
  auto value = ctx->constructObject(ctx->Error(), {}, {});
  if (!args.empty()) {
    value = args[0];
  }
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"value"] = value;
  auto onCreateRejectedFunc =
      ctx->createNativeFunction(onCreateRejected, closure);
  return ctx->constructObject(ctx->Promise(), {onCreateRejectedFunc}, {});
}

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
  auto resolveFunc = ctx->createNativeFunction(::resolve);
  auto rejectFunc = ctx->createNativeFunction(::reject);
  resolveFunc->setBind(ctx, self);
  rejectFunc->setBind(ctx, self);
  auto err = resolver->apply(ctx, ctx->undefined(), {resolveFunc, rejectFunc});
  if (err->isException()) {
    return err;
  }
  return ctx->undefined();
}

JS_FUNC(JSPromiseConstructor::then) {
  auto entity = self->getEntity<JSPromiseEntity>();
  auto callback = ctx->undefined();
  auto onError = ctx->undefined();
  if (args.size() > 0) {
    callback = args[0];
  }
  if (args.size() > 1) {
    onError = args[1];
  }
  if (entity->getStatus() == JSPromiseEntity::Status::FULFILLED) {
    auto value = ctx->createValue(entity->getValue());
    if (callback->isFunction()) {
      common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
      closure[L"callback"] = callback;
      closure[L"value"] = value;
      auto nextCallbackFunc = ctx->createNativeFunction(nextCallback, closure);
      return ctx->constructObject(ctx->Promise(), {nextCallbackFunc}, {});
    } else {
      return ctx->Promise()
          ->getProperty(ctx, L"resolve")
          ->apply(ctx, ctx->undefined(), {value});
    }
  } else if (entity->getStatus() == JSPromiseEntity::Status::REJECTED) {
    auto value = ctx->createValue(entity->getValue());
    if (callback->isFunction()) {
      common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
      closure[L"callback"] = onError;
      closure[L"value"] = value;
      auto nextCallbackFunc = ctx->createNativeFunction(nextCallback, closure);
      return ctx->constructObject(ctx->Promise(), {nextCallbackFunc}, {});
    } else {
      return ctx->Promise()
          ->getProperty(ctx, L"reject")
          ->apply(ctx, ctx->undefined(), {value});
    }
  } else {
    common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
    closure[L"callback"] = callback;
    closure[L"onError"] = onError;
    auto nextPenddingFunc = ctx->createNativeFunction(nextPendding, closure);
    nextPenddingFunc->setBind(ctx, self);
    return ctx->constructObject(ctx->Promise(), {nextPenddingFunc}, {});
  }
}
JS_FUNC(JSPromiseConstructor::catch_) {
  auto entity = self->getEntity<JSPromiseEntity>();
  auto callback = ctx->undefined();
  if (!args.empty()) {
    callback = args[0];
  }
  if (entity->getStatus() == JSPromiseEntity::Status::REJECTED) {
    auto value = ctx->createValue(entity->getValue());
    if (callback->isFunction()) {
      common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
      closure[L"callback"] = callback;
      closure[L"value"] = value;
      auto nextCallbackFunc = ctx->createNativeFunction(nextCallback, closure);
      return ctx->constructObject(ctx->Promise(), {nextCallbackFunc}, {});
    } else {
      return ctx->Promise()
          ->getProperty(ctx, L"reject")
          ->apply(ctx, ctx->undefined(), {value});
    }
  } else if (entity->getStatus() == JSPromiseEntity::Status::PENDING) {
    common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
    closure[L"onError"] = callback;
    closure[L"callback"] = ctx->undefined();
    auto nextPenddingFunc = ctx->createNativeFunction(nextPendding, closure);
    nextPenddingFunc->setBind(ctx, self);
    return ctx->constructObject(ctx->Promise(), {nextPenddingFunc}, {});
  } else {
    auto value = ctx->createValue(entity->getValue());
    return ctx->Promise()
        ->getProperty(ctx, L"resolve")
        ->apply(ctx, ctx->undefined(), {value});
  }
  return ctx->undefined();
}
JS_FUNC(JSPromiseConstructor::finally) { return ctx->undefined(); }

common::AutoPtr<JSValue>
JSPromiseConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto Promise = ctx->createNativeFunction(constructor, L"Promise", L"Promise");
  auto prototype = ctx->createObject();
  prototype->setPropertyDescriptor(ctx, L"constructor", Promise);
  Promise->setPropertyDescriptor(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(ctx, L"then",
                                   ctx->createNativeFunction(then, L"then"));
  prototype->setPropertyDescriptor(ctx, L"catch",
                                   ctx->createNativeFunction(catch_, L"catch"));
  prototype->setPropertyDescriptor(
      ctx, L"finally", ctx->createNativeFunction(finally, L"finally"));

  Promise->setPropertyDescriptor(
      ctx, L"resolve", ctx->createNativeFunction(resolve, L"resolve"));

  Promise->setPropertyDescriptor(ctx, L"reject",
                                 ctx->createNativeFunction(reject, L"resolve"));
  return Promise;
}