#include "engine/entity/JSObjectEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::engine;
JSObjectEntity::JSObjectEntity(JSEntity *prototype)
    : JSEntity(JSValueType::JS_OBJECT), _prototype(prototype),
      _extensible(true), _sealed(false), _frozen(false) {
  appendChild(prototype);
}
JSEntity *JSObjectEntity::getPrototype() const { return _prototype; }

bool JSObjectEntity::isExtensible() const { return _extensible; }

bool JSObjectEntity::isSealed() const { return _sealed; }

bool JSObjectEntity::isFrozen() const { return _frozen; }

void JSObjectEntity::preventExtensions() { _extensible = false; }

void JSObjectEntity::seal() { _sealed = true; }

void JSObjectEntity::freeze() { _frozen = true; }

const std::unordered_map<JSEntity *, JSObjectEntity::JSField> &
JSObjectEntity::getSymbolProperties() const {
  return _symbolFields;
}
const std::unordered_map<std::wstring, JSObjectEntity::JSField> &
JSObjectEntity::getProperties() const {
  return _fields;
}
std::unordered_map<JSEntity *, JSObjectEntity::JSField> &
JSObjectEntity::getSymbolProperties() {
  return _symbolFields;
}
std::unordered_map<std::wstring, JSObjectEntity::JSField> &
JSObjectEntity::getProperties() {
  return _fields;
}

std::wstring JSObjectEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"[object Object]";
}

std::optional<double>
JSObjectEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return std::nullopt;
}

bool JSObjectEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
}
