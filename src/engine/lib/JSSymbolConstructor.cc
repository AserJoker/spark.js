#include "engine/lib/JSSymbolConstructor.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "error/JSTypeError.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;

JS_FUNC(JSSymbolConstructor::constructor) {
  if (args.empty()) {
    return ctx->createSymbol();
  } else {
    return ctx->createSymbol(args[0]->convertToString(ctx));
  }
}

JS_FUNC(JSSymbolConstructor::toString) {
  if (self == nullptr) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  auto symbol = self;
  if (self->getType() == JSValueType::JS_OBJECT) {
    symbol = self->toPrimitive(ctx);
  }
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
  auto symbol = self;
  if (self->getType() == JSValueType::JS_OBJECT) {
    symbol = self->getProperty(ctx, ctx->symbolValue());
  }
  if (symbol->getType() != JSValueType::JS_SYMBOL) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  return symbol;
}

JS_FUNC(JSSymbolConstructor::_for) {
  std::wstring key;
  if (args.empty()) {
    key = ctx->undefined()->convertToString(ctx);
  } else {
    key = args[0]->convertToString(ctx);
  }
  auto &symbols =
      ctx->Symbol()->getOpaque<common::AutoPtr<JSSymbolOpaque>>()->symbols;
  if (symbols.contains(key)) {
    return ctx->createValue(symbols.at(key));
  }
  auto value = ctx->createSymbol(key);
  ctx->Symbol()->getEntity()->appendChild(value->getEntity());
  symbols[key] = value->getEntity();
  return value;
}

JS_FUNC(JSSymbolConstructor::forKey) {
  std::wstring key;
  if (args.empty()) {
    key = ctx->undefined()->convertToString(ctx);
  } else {
    key = args[0]->convertToString(ctx);
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
  auto symbol = self;
  if (symbol->getType() == JSValueType::JS_OBJECT) {
    symbol = self->toPrimitive(ctx);
  }
  if (symbol->getType() != JSValueType::JS_SYMBOL) {
    throw error::JSTypeError(
        L"Symbol.prototype.description requires that 'this' be a Symbol");
  }
  return ctx->createString(
      symbol->getEntity<JSSymbolEntity>()->getDescription());
}

void JSSymbolConstructor::initialize(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> Symbol) {

  auto proto = Symbol->getProperty(ctx, L"prototype");

  Symbol->setProperty(ctx, L"asyncIterator",
                      ctx->createSymbol(L"Symbol.asyncIterator"));

  Symbol->setProperty(ctx, L"hasInstance",
                      ctx->createSymbol(L"Symbol.hasInstance"));

  Symbol->setProperty(ctx, L"isConcatSpreadable",
                      ctx->createSymbol(L"Symbol.isConcatSpreadable"));

  Symbol->setProperty(ctx, L"iterator", ctx->createSymbol(L"Symbol.iterator"));

  Symbol->setProperty(ctx, L"match", ctx->createSymbol(L"Symbol.matchAll"));

  Symbol->setProperty(ctx, L"replace", ctx->createSymbol(L"Symbol.replace"));

  Symbol->setProperty(ctx, L"search", ctx->createSymbol(L"Symbol.search"));

  Symbol->setProperty(ctx, L"species", ctx->createSymbol(L"Symbol.species"));

  Symbol->setProperty(ctx, L"split", ctx->createSymbol(L"Symbol.split"));

  Symbol->setProperty(ctx, L"toPrimitive",
                      ctx->createSymbol(L"Symbol.toPrimitive"));

  Symbol->setProperty(ctx, L"toStringTag",
                      ctx->createSymbol(L"Symbol.toStringTag"));

  Symbol->setProperty(ctx, L"unscopables",
                      ctx->createSymbol(L"Symbol.unscopables"));

  proto->setProperty(
      ctx, L"toString",
      ctx->createFunction(&JSSymbolConstructor::toString, L"toString"));

  proto->setProperty(
      ctx, L"valueOf",
      ctx->createFunction(&JSSymbolConstructor::valueOf, L"valueOf"));

  proto->setProperty(ctx, L"for",
                     ctx->createFunction(&JSSymbolConstructor::_for, L"for"));

  proto->setProperty(
      ctx, L"forKey",
      ctx->createFunction(&JSSymbolConstructor::forKey, L"forKey"));

  proto->setPropertyDescriptor(
      ctx, L"description",
      JSObjectEntity::JSField{
          .configurable = false,
          .enumable = false,
          .value = nullptr,
          .writable = false,
          .get = ctx->createFunction(&JSSymbolConstructor::description)
                     ->getEntity(),
          .set = nullptr});

  Symbol->setOpaque<common::AutoPtr<JSSymbolOpaque>>(new JSSymbolOpaque);
}