#include "engine/lib/JSSymbolConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "error/JSTypeError.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSSymbolConstructor::constructor) {
  if (args.empty()) {
    return ctx->createSymbol();
  } else {
    return ctx->createSymbol(args[0]->toString(ctx)->getString().value());
  }
}

JS_FUNC(JSSymbolConstructor::toString) {
  if (self == nullptr) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  auto symbol = self->toPrimitive(ctx);
  if (symbol->getType() != JSValueType::JS_SYMBOL) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  return ctx->createString(fmt::format(
      L"Symbol({})", symbol->getEntity<JSSymbolEntity>()->getDescription()));
}

JS_FUNC(JSSymbolConstructor::valueOf) {
  if (self == nullptr) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  auto symbol = self->getProperty(ctx, ctx->internalSymbol(L"value"));

  if (symbol->getType() != JSValueType::JS_SYMBOL) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  return symbol;
}

JS_FUNC(JSSymbolConstructor::_for) {
  std::wstring key;
  if (args.empty()) {
    key = ctx->undefined()->toString(ctx)->getString().value();
  } else {
    key = args[0]->toString(ctx)->getString().value();
  }
  auto &symbols =
      ctx->Symbol()->getOpaque<common::AutoPtr<JSSymbolOpaque>>()->symbols;
  if (symbols.contains(key)) {
    return ctx->createValue(symbols.at(key));
  }
  auto value = ctx->createSymbol(key);
  ctx->Symbol()->getStore()->appendChild(value->getStore());
  symbols[key] = value->getStore();
  return value;
}

JS_FUNC(JSSymbolConstructor::forKey) {
  std::wstring key;
  if (args.empty()) {
    key = ctx->undefined()->toString(ctx)->getString().value();
  } else {
    key = args[0]->toString(ctx)->getString().value();
  }
  auto &symbols =
      ctx->Symbol()->getOpaque<common::AutoPtr<JSSymbolOpaque>>()->symbols;
  for (auto &[k, v] : symbols) {
    if (k == key) {
      return ctx->createValue(v);
    }
  }
  return ctx->undefined();
}
JS_FUNC(JSSymbolConstructor::description) {
  if (self == nullptr) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  auto symbol = self->toPrimitive(ctx);
  if (symbol->getType() != JSValueType::JS_SYMBOL) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  return ctx->createString(
      symbol->getEntity<JSSymbolEntity>()->getDescription());
}

JS_FUNC(JSSymbolConstructor::toPrimitive) {
  if (self == nullptr) {
    throw error::JSTypeError(
        L"Symbol.prototype [ @@toPrimitive ] requires that 'this' be a Symbol");
  }
  auto symbol = self->getProperty(ctx, ctx->internalSymbol(L"value"));
  if (symbol->getType() != JSValueType::JS_SYMBOL) {
    throw error::JSTypeError(
        L"Symbol.prototype [ @@toPrimitive ] requires that 'this' be a Symbol");
  }
  return symbol;
}

void JSSymbolConstructor::initialize(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> Symbol,
                                     common::AutoPtr<JSValue> prototype) {

  Symbol->setPropertyDescriptor(ctx, L"asyncIterator",
                                ctx->createSymbol(L"Symbol.asyncIterator"));

  Symbol->setPropertyDescriptor(ctx, L"hasInstance",
                                ctx->createSymbol(L"Symbol.hasInstance"));

  Symbol->setPropertyDescriptor(
      ctx, L"isConcatSpreadable",
      ctx->createSymbol(L"Symbol.isConcatSpreadable"));

  Symbol->setPropertyDescriptor(ctx, L"iterator",
                                ctx->createSymbol(L"Symbol.iterator"));

  Symbol->setPropertyDescriptor(ctx, L"match",
                                ctx->createSymbol(L"Symbol.matchAll"));

  Symbol->setPropertyDescriptor(ctx, L"replace",
                                ctx->createSymbol(L"Symbol.replace"));

  Symbol->setPropertyDescriptor(ctx, L"search",
                                ctx->createSymbol(L"Symbol.search"));

  Symbol->setPropertyDescriptor(ctx, L"species",
                                ctx->createSymbol(L"Symbol.species"));

  Symbol->setPropertyDescriptor(ctx, L"split",
                                ctx->createSymbol(L"Symbol.split"));

  auto toPrimitive = ctx->createSymbol(L"Symbol.toPrimitive");

  Symbol->setPropertyDescriptor(ctx, L"toPrimitive", toPrimitive);

  auto toStringTag = ctx->createSymbol(L"Symbol.toStringTag");

  Symbol->setPropertyDescriptor(ctx, L"toStringTag", toStringTag);

  Symbol->setPropertyDescriptor(ctx, L"unscopables",
                                ctx->createSymbol(L"Symbol.unscopables"));

  prototype->setPropertyDescriptor(
      ctx, L"toString",
      ctx->createNativeFunction(&JSSymbolConstructor::toString, L"toString"));

  prototype->setPropertyDescriptor(
      ctx, L"valueOf",
      ctx->createNativeFunction(&JSSymbolConstructor::valueOf, L"valueOf"));

  prototype->setPropertyDescriptor(
      ctx, L"for",
      ctx->createNativeFunction(&JSSymbolConstructor::_for, L"for"));

  prototype->setPropertyDescriptor(
      ctx, L"forKey",
      ctx->createNativeFunction(&JSSymbolConstructor::forKey, L"forKey"));

  prototype->setPropertyDescriptor(
      ctx, L"description",
      ctx->createNativeFunction(&JSSymbolConstructor::description), nullptr);

  prototype->setPropertyDescriptor(
      ctx, toPrimitive,
      ctx->createNativeFunction(&JSSymbolConstructor::toPrimitive,
                                L"[Symbol.toPrimitive]"));

  prototype->setPropertyDescriptor(ctx, toStringTag,
                                   ctx->createString(L"Symbol"));

  Symbol->setOpaque<common::AutoPtr<JSSymbolOpaque>>(new JSSymbolOpaque);
}