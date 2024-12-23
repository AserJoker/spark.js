#include "engine/lib/JSArrayConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSArrayEntity.hpp"
#include "engine/runtime/JSStore.hpp"
#include "error/JSTypeError.hpp"
#include <string>
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSArrayConstructor::constructor) {
  JSStore *store = nullptr;
  if (self == nullptr) {
    store = new JSStore(new JSArrayEntity(
        ctx->Array()->getProperty(ctx, L"prototype")->getStore()));
    self = ctx->createValue(store);
  } else {
    store = self->getStore();
  }
  if (args.size() == 1 && args[0]->getType() == JSValueType::JS_NUMBER) {
    int64_t length = (int64_t)args[0]->getNumber().value();
    int64_t index = 0;
    while (index < length) {
      self->setIndex(ctx, index, ctx->undefined());
      index++;
    }
  } else {
    for (size_t index = 0; index < args.size(); index++) {
      self->setIndex(ctx, index, args[index]);
    }
  }
  return self;
}

JS_FUNC(JSArrayConstructor::getLength) {
  if (!self || self->getType() != JSValueType::JS_ARRAY) {
    return ctx->createNumber(0);
  }
  return ctx->createNumber(self->getEntity<JSArrayEntity>()->getItems().size());
}

JS_FUNC(JSArrayConstructor::setLength) {
  if (!self || self->getType() != JSValueType::JS_ARRAY) {
    return ctx->createNumber(0);
  }
  if (args.size() < 1 || args[0]->getType() != JSValueType::JS_NUMBER) {
    throw error::JSTypeError(L"set length require a 'number' argument");
  }
  auto len = (int64_t)args[0]->getNumber().value();
  auto store = self->getStore();
  auto entity = self->getEntity<JSArrayEntity>();
  while (entity->getItems().size() > len) {
    auto e = *entity->getItems().rbegin();
    ctx->getScope()->getRoot()->appendChild(e);
    store->removeChild(e);
    entity->getItems().pop_back();
  }
  while (entity->getItems().size() < len) {
    auto e = ctx->undefined()->getStore();
    entity->getItems().push_back(e);
    store->appendChild(e);
  }
  return getLength(ctx, self, {});
}

JS_FUNC(JSArrayConstructor::toStringTag) { return ctx->createString(L"Array"); }

JS_FUNC(JSArrayConstructor::join) {
  std::wstring separator = L",";
  if (args.size() > 0) {
    separator = args[0]->toString(ctx)->getString().value();
  }
  auto len = self->getProperty(ctx, L"length")->toNumber(ctx)->getNumber();
  if (!len.has_value()) {
    return ctx->createString();
  }
  std::wstring result;
  auto length = (int64_t)len.value();
  for (size_t index = 0; index < length; index++) {
    auto item = self->getProperty(ctx, ctx->createNumber(index));
    if (!item->isNull() && !item->isUndefined()) {
      result += item->toString(ctx)->getString().value();
    }
    if (index != length - 1) {
      result += separator;
    }
  }
  return ctx->createString(result);
}

JS_FUNC(JSArrayConstructor::values) {
  auto obj = ctx->createObject();
  auto index = ctx->createNumber();
  obj->setOpaque(JSArrayIteratorContext{
      .value = nullptr,
      .array = self,
      .index = index,
  });
  obj->getStore()->appendChild(self->getStore());
  obj->getStore()->appendChild(index->getStore());

  obj->setProperty(ctx, L"next",
                   ctx->createNativeFunction(iterator_next, L"next"));
  return obj;
}

JS_FUNC(JSArrayConstructor::push) {
  auto len = self->getProperty(ctx, L"length");
  for (auto &arg : args) {
    self->setProperty(ctx, len, arg);
    len->increment(ctx);
  }
  return len;
}
JS_FUNC(JSArrayConstructor::forEach) {
  auto len = self->getProperty(ctx, L"length");
  auto index = ctx->createNumber();
  while (index->lt(ctx, len)->getBoolean().value()) {
    args[0]->apply(ctx, ctx->undefined(),
                   {self->getProperty(ctx, index), index, self});
    index->increment(ctx);
  }
  return ctx->undefined();
}
JS_FUNC(JSArrayConstructor::map) {
  auto result = ctx->createArray();
  auto len = self->getProperty(ctx, L"length");
  auto index = ctx->createNumber();
  while (index->lt(ctx, len)->getBoolean().value()) {
    auto res = args[0]->apply(ctx, ctx->undefined(),
                              {self->getProperty(ctx, index), index, self});
    result->setProperty(ctx, index, res);
    index->increment(ctx);
  }
  return result;
}

JS_FUNC(JSArrayConstructor::iterator_next) {
  if (!self->hasOpaque<JSArrayIteratorContext>()) {
    throw error::JSTypeError(
        fmt::format(L" Method Array Iterator.prototype.next called on "
                    L"incompatible receiver {}",
                    self->toString(ctx)->getString().value()));
  }
  auto ictx = self->getOpaque<JSArrayIteratorContext>();
  auto result = ctx->createObject();
  if (ictx.value != nullptr) {
    result->setProperty(ctx, L"done", ctx->truly());
  }
  auto length = ictx.array->getProperty(ctx, L"length");
  if (ictx.index->lt(ctx, length)->getBoolean().value()) {
    result->setProperty(ctx, L"value",
                        ictx.array->getProperty(ctx, ictx.index));
    ictx.index->increment(ctx);
  } else {
    ictx.value = ctx->undefined();
    result->setProperty(ctx, L"done", ctx->truly());
  }
  return result;
}

JS_FUNC(JSArrayConstructor::toString) {
  auto join = self->getProperty(ctx, L"join");
  if (join->getType() == JSValueType::JS_FUNCTION ||
      join->getType() == JSValueType::JS_NATIVE_FUNCTION) {
    return join->apply(ctx, self);
  }
  return ctx->Object()
      ->getProperty(ctx, L"prototype")
      ->getProperty(ctx, L"toString")
      ->apply(ctx, self);
}

common::AutoPtr<JSValue>
JSArrayConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto Array = ctx->createNativeFunction(constructor, L"Array", L"Array");
  ctx->pushScope();
  auto prototype = ctx->createObject();

  prototype->setPropertyDescriptor(ctx, L"constructor", Array, true, false);

  Array->setPropertyDescriptor(ctx, L"prototype", prototype, true, false);

  prototype->setPropertyDescriptor(
      ctx, L"length", ctx->createNativeFunction(getLength),
      ctx->createNativeFunction(setLength), true, false);

  prototype->setPropertyDescriptor(
      ctx, ctx->Symbol()->getProperty(ctx, L"toStringTag"),
      ctx->createNativeFunction(toStringTag, L"[Symbol.toStringTag]"), true,
      false);

  prototype->setPropertyDescriptor(
      ctx, L"toString", ctx->createNativeFunction(toString, L"toString"), true,
      false);

  prototype->setPropertyDescriptor(
      ctx, L"join", ctx->createNativeFunction(join, L"join"), true, false);

  prototype->setPropertyDescriptor(
      ctx, L"push", ctx->createNativeFunction(push, L"push"), true, false);

  prototype->setPropertyDescriptor(
      ctx, L"forEach", ctx->createNativeFunction(forEach, L"forEach"), true,
      false);
  
  prototype->setPropertyDescriptor(
      ctx, L"map", ctx->createNativeFunction(map, L"map"), true, false);

  auto values =
      ctx->createNativeFunction(JSArrayConstructor::values, L"values");

  prototype->setPropertyDescriptor(ctx, L"values", values, true, false);

  prototype->setPropertyDescriptor(
      ctx, ctx->Symbol()->getProperty(ctx, L"iterator"), values, true, false);
  ctx->popScope();
  return Array;
}