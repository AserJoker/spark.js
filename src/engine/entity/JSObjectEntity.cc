#include "engine/entity/JSObjectEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;
JSObjectEntity::JSObjectEntity(JSStore *prototype)
    : JSEntity(JSValueType::JS_OBJECT), _prototype(prototype),
      _extensible(true), _sealed(false), _frozen(false) {}

const JSStore *JSObjectEntity::getPrototype() const { return _prototype; }

JSStore *JSObjectEntity::getPrototype() { return _prototype; }

bool JSObjectEntity::isExtensible() const { return _extensible; }

bool JSObjectEntity::isSealed() const { return _sealed; }

bool JSObjectEntity::isFrozen() const { return _frozen; }

void JSObjectEntity::preventExtensions() { _extensible = false; }

void JSObjectEntity::seal() { _sealed = true; }

void JSObjectEntity::freeze() { _frozen = true; }

const std::unordered_map<JSStore *, JSObjectEntity::JSField> &
JSObjectEntity::getSymbolProperties() const {
  return _symbolFields;
}
const std::unordered_map<std::wstring, JSObjectEntity::JSField> &
JSObjectEntity::getProperties() const {
  return _fields;
}
std::unordered_map<JSStore *, JSObjectEntity::JSField> &
JSObjectEntity::getSymbolProperties() {
  return _symbolFields;
}
std::unordered_map<std::wstring, JSObjectEntity::JSField> &
JSObjectEntity::getProperties() {
  return _fields;
}

std::wstring JSObjectEntity::toString(common::AutoPtr<JSContext> ctx) const {
  std::wstring str = L"Object";
  auto symbol = ctx->Symbol()->getProperty(ctx, L"toStringTag");
  if (_symbolFields.contains(symbol->getStore())) {
    str =
        ctx->getScope()
            ->createValue((JSStore *)_symbolFields.at(symbol->getStore()).value)
            ->convertToString(ctx);
  }
  return fmt::format(L"[object {}]", str);
}

std::optional<double>
JSObjectEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return std::nullopt;
}

bool JSObjectEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
}
