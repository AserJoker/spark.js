#include "engine/lib/JSArrayConstructor.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSArrayEntity.hpp"
#include "error/JSTypeError.hpp"
#include <string>
using namespace spark;
using namespace spark::engine;
JS_FUNC(JSArrayConstructor::constructor) {
  JSArrayEntity *entity = nullptr;
  if (self == nullptr) {
    entity = new JSArrayEntity(
        ctx->Array()->getProperty(ctx, L"prototype")->getEntity());
    self = ctx->createValue(entity);
  } else {
    entity = self->getEntity<JSArrayEntity>();
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
  auto entity = self->getEntity<JSArrayEntity>();
  while (entity->getItems().size() > len) {
    auto e = *entity->getItems().rbegin();
    ctx->getScope()->getRoot()->appendChild(e);
    entity->removeChild(e);
    entity->getItems().pop_back();
  }
  while (entity->getItems().size() < len) {
    auto e = ctx->undefined()->getEntity();
    entity->getItems().push_back(e);
    entity->appendChild(e);
  }
  return getLength(ctx, self, {});
}

JS_FUNC(JSArrayConstructor::toStringTag) { return ctx->createString(L"Array"); }

JS_FUNC(JSArrayConstructor::join) {
  std::wstring separator = L",";
  if (args.size() > 0) {
    separator = args[0]->convertToString(ctx);
  }
  auto len = self->getProperty(ctx, L"length")->convertToNumber(ctx);
  if (!len.has_value()) {
    return ctx->createString();
  }
  std::wstring result;
  auto length = (int64_t)len.value();
  for (size_t index = 0; index < length; index++) {
    auto item = self->getProperty(ctx, ctx->createNumber(index));
    if (!item->isNull() && !item->isUndefined()) {
      result += item->convertToString(ctx);
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
  return ctx->Object()->getProperty(ctx, L"toString")->apply(ctx, self);
}

common::AutoPtr<JSValue>
JSArrayConstructor::initialize(common::AutoPtr<JSContext> ctx) {
  auto prototype = ctx->createObject();
  auto Array = ctx->createNativeFunction(constructor, L"Array", L"Array");
  prototype->setProperty(ctx, L"constructor", Array);
  Array->setProperty(ctx, L"prototype", prototype);
  prototype->setPropertyDescriptor(
      ctx, L"length",
      {
          .configurable = true,
          .enumable = false,
          .get = ctx->createNativeFunction(getLength)->getEntity(),
          .set = ctx->createNativeFunction(setLength)->getEntity(),
      });
  prototype->setProperty(
      ctx, ctx->Symbol()->getProperty(ctx, L"Symbol.toStringTag"),
      ctx->createNativeFunction(toStringTag, L"[Symbol.toStringTag]"));
  prototype->setProperty(ctx, L"toString", ctx->createNativeFunction(toString));
  prototype->setProperty(ctx, L"join", ctx->createNativeFunction(join));
  return Array;
}