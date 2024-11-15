#include "engine/runtime/JSValue.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSBooleanEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/entity/JSNumberEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSScope.hpp"
#include "error/JSInternalError.hpp"
#include <codecvt>
#include <exception>
#include <fmt/xchar.h>
#include <locale>

using namespace spark;
using namespace spark::engine;
JSValue::JSValue(JSScope *scope, JSEntity *entity)
    : _scope(scope), _entity(entity) {}

JSValue::~JSValue() {}

JSEntity *JSValue::getEntity() { return _entity; }

const JSEntity *JSValue::getEntity() const { return _entity; }

const JSValueType &JSValue::getType() const {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  return _entity->getType();
}

void JSValue::setEntity(JSEntity *entity) { _entity = entity; }

void JSValue::setMetatable(const common::AutoPtr<JSValue> &meta) {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  _entity->setMetatable((JSEntity *)meta->getEntity());
}

const common::AutoPtr<JSValue> JSValue::getMetatable() const {
  if (_entity) {
    return _scope->createValue(_entity->getMetatable());
  }
  return nullptr;
}

std::optional<double> JSValue::getNumber() const {
  if (getType() == JSValueType::JS_NUMBER) {
    return ((JSNumberEntity *)_entity)->getValue();
  }
  return std::nullopt;
}

std::optional<std::wstring> JSValue::getString() const {
  if (getType() == JSValueType::JS_STRING) {
    return ((JSStringEntity *)_entity)->getValue();
  }
  return std::nullopt;
}

std::optional<bool> JSValue::getBoolean() const {
  if (getType() == JSValueType::JS_BOOLEAN) {
    return ((JSBooleanEntity *)_entity)->getValue();
  }
  return std::nullopt;
}

bool JSValue::isUndefined() const {
  return getType() == JSValueType::JS_UNDEFINED;
}

bool JSValue::isNull() const { return getType() == JSValueType::JS_NULL; }

bool JSValue::isInifity() const {
  return getType() == JSValueType::JS_INFINITY;
}

bool JSValue::isNaN() const { return getType() == JSValueType::JS_NAN; }

void JSValue::setNumber(double value) {
  if (_entity->getType() != JSValueType::JS_NUMBER) {
    _entity = new JSNumberEntity(value);
    _scope->getRoot()->appendChild(_entity);
  } else {
    ((JSNumberEntity *)_entity)->getValue() = value;
  }
}

void JSValue::setString(const std::wstring &value) {
  if (_entity->getType() != JSValueType::JS_STRING) {
    _entity = new JSStringEntity(value);
    _scope->getRoot()->appendChild(_entity);
  } else {
    ((JSStringEntity *)_entity)->getValue() = value;
  }
}

void JSValue::setBoolean(bool value) {
  if (_entity->getType() != JSValueType::JS_BOOLEAN) {
    _entity = new JSBooleanEntity(value);
    _scope->getRoot()->appendChild(_entity);
  } else {
    ((JSBooleanEntity *)_entity)->getValue() = value;
  }
}
void JSValue::setUndefined() {
  if (_entity->getType() != JSValueType::JS_UNDEFINED) {
    _entity = _scope->getRootScope()->getValue(L"undefined")->getEntity();
  }
}

void JSValue::setNull() {
  if (_entity->getType() != JSValueType::JS_NULL) {
    _entity = _scope->getValue(L"null")->getEntity();
  }
}

void JSValue::setInfinity() {
  if (_entity->getType() != JSValueType::JS_INFINITY) {
    _entity = _scope->getValue(L"Infinity")->getEntity();
  }
}

void JSValue::setNaN() {
  if (_entity->getType() != JSValueType::JS_NAN) {
    _entity = _scope->getValue(L"NaN")->getEntity();
  }
}

common::AutoPtr<JSValue>
JSValue::apply(common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> self,
               common::Array<common::AutoPtr<JSValue>> args,
               const JSLocation &location) {
  if (getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto entity = (JSNativeFunctionEntity *)_entity;
    ctx->pushCallStack(entity->getFunctionName(), location);
    auto scope = ctx->pushScope();
    JSEntity *result = nullptr;
    try {
      result = entity->apply(ctx, self, args)->getEntity();
    } catch (error::JSError &e) {
      result = ctx->createException(e.getType(), e.getMessage())->getEntity();
    } catch (std::exception &e) {
      static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      result = ctx->createException(L"InternalError",
                                    converter.from_bytes(e.what()), location)
                   ->getEntity();
    }
    scope->getRoot()->appendChild(result);
    ctx->popScope(scope);
    ctx->popCallStack();
    return ctx->createValue(result);
  }
  return ctx->createException(L"TypeError", L"not a function", location);
}

std::wstring JSValue::toStringValue(common::AutoPtr<JSContext> ctx) {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  return _entity->toString(ctx);
}

std::optional<double> JSValue::toNumberValue(common::AutoPtr<JSContext> ctx) {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  return _entity->toNumber(ctx);
}

bool JSValue::toBooleanValue(common::AutoPtr<JSContext> ctx) {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  return _entity->toBoolean(ctx);
}

std::wstring JSValue::getTypeName(common::AutoPtr<JSContext> ctx) {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  return _entity->getTypeName(ctx);
}

common::AutoPtr<JSValue> JSValue::toNumber(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_BIGINT) {
    return this;
  }
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->Infinity();
  }
  auto val = toNumberValue(ctx);
  if (val.has_value()) {
    return ctx->createNumber(val.value());
  } else {
    return ctx->NaN();
  }
}

common::AutoPtr<JSValue> JSValue::toString(common::AutoPtr<JSContext> ctx) {
  return ctx->createString(toStringValue(ctx));
}

common::AutoPtr<JSValue> JSValue::toBoolean(common::AutoPtr<JSContext> ctx) {
  return ctx->createBoolean(toBooleanValue(ctx));
}