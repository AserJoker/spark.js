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
  prototype->setProperty(ctx, L"constructor", Array);
  Array->setProperty(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(
      ctx, L"length",
      {
          .configurable = true,
          .enumable = false,
          .get = ctx->createNativeFunction(getLength)->getStore(),
          .set = ctx->createNativeFunction(setLength)->getStore(),
      });
  prototype->setPropertyDescriptor(
      ctx, ctx->Symbol()->getProperty(ctx, L"toStringTag"),
      {.configurable = true,
       .enumable = false,
       .get = ctx->createNativeFunction(toStringTag, L"[Symbol.toStringTag]")
                  ->getStore(),
       .set = nullptr});
  prototype->setProperty(ctx, L"toString",
                         ctx->createNativeFunction(toString, L"toString"));
  prototype->setProperty(ctx, L"join",
                         ctx->createNativeFunction(join, L"join"));
  ctx->popScope();
  return Array;
}