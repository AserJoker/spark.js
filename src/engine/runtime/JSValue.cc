#include "engine/runtime/JSValue.hpp"
#include "common/AutoPtr.hpp"
#include "common/BigInt.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSArrayEntity.hpp"
#include "engine/entity/JSBigIntEntity.hpp"
#include "engine/entity/JSBooleanEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/entity/JSNumberEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSScope.hpp"
#include "error/JSError.hpp"
#include "error/JSInternalError.hpp"
#include "error/JSRangeError.hpp"
#include "error/JSReferenceError.hpp"
#include "error/JSTypeError.hpp"
#include <codecvt>
#include <exception>
#include <fmt/xchar.h>
#include <locale>
#include <string>

using namespace spark;
using namespace spark::engine;
JSValue::JSValue(JSScope *scope, JSEntity *entity)
    : _scope(scope), _entity(entity) {}

JSValue::~JSValue() {}

const JSValueType &JSValue::getType() const {
  if (!_entity) {
    throw error::JSInternalError(L"out of scope");
  }
  return _entity->getType();
}
std::wstring JSValue::getName() const {
  for (auto &[name, val] : _scope->getValues()) {
    if (val == this) {
      return name;
    }
  }
  return L"anonymous";
}

void JSValue::setEntity(JSEntity *entity) { _entity = entity; }

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

std::optional<common::BigInt<>> JSValue::getBigInt() const {
  if (getType() == JSValueType::JS_BIGINT) {
    return getEntity<JSBigIntEntity>()->getValue();
  }
  return std::nullopt;
}

bool JSValue::isUndefined() const {
  return getType() == JSValueType::JS_UNDEFINED;
}

bool JSValue::isNull() const { return getType() == JSValueType::JS_NULL; }

bool JSValue::isInfinity() const {
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
               std::vector<common::AutoPtr<JSValue>> args,
               const JSLocation &location) {
  common::AutoPtr<JSValue> func = this;
  if (getType() == JSValueType::JS_OBJECT) {
    func = self->toPrimitive(ctx);
  }
  if (func->getType() != JSValueType::JS_NATIVE_FUNCTION &&
      func->getType() != JSValueType::JS_FUNCTION) {

    throw error::JSTypeError(fmt::format(L"{} is not a function", getName()),
                             location);
  }
  ctx->pushCallStack(location);
  auto scope = ctx->pushScope();
  JSEntity *result = nullptr;
  try {
    auto vm = ctx->getRuntime()->getVirtualMachine();
    result = vm->apply(ctx, func, self, args)->getEntity();
  } catch (error::JSError &e) {
    result = ctx->createException(e.getType(), e.getMessage(), location)
                 ->getEntity();
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

common::AutoPtr<JSValue> JSValue::toPrimitive(common::AutoPtr<JSContext> ctx,
                                              const std::wstring &hint) {
  if (getType() < JSValueType::JS_OBJECT) {
    return this;
  }
  auto toPrimitive =
      getProperty(ctx, ctx->Symbol()->getProperty(ctx, L"toPrimitive"));
  if (toPrimitive->getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto res = toPrimitive->apply(ctx, this, {ctx->createString(L"default")});
    if (res->getType() < JSValueType::JS_OBJECT) {
      return res;
    }
  }
  auto getter = getProperty(ctx, L"valueOf");
  if (getter->getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto res = getter->apply(ctx, this);
    if (res->getType() < JSValueType::JS_OBJECT) {
      return res;
    }
  }
  getter = getProperty(ctx, L"toString");
  if (getter->getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto res = getter->apply(ctx, this);
    if (res->getType() < JSValueType::JS_OBJECT) {
      return res;
    }
  }
  throw error::JSTypeError(
      fmt::format(L"Cannot convert {} to primitive value", getTypeName()));
}

common::AutoPtr<JSValue> JSValue::pack(common::AutoPtr<JSContext> ctx) {
  switch (getType()) {
  case JSValueType::JS_NUMBER:
    break;
  case JSValueType::JS_BIGINT:
    break;
  case JSValueType::JS_NAN:
    break;
  case JSValueType::JS_INFINITY:
    break;
  case JSValueType::JS_STRING:
    break;
  case JSValueType::JS_BOOLEAN:
    break;
  case JSValueType::JS_SYMBOL: {
    auto val = ctx->createObject(ctx->Symbol()->getProperty(ctx, L"prototype"));
    val->setProperty(ctx, ctx->symbolValue(), this);
    val->setProperty(ctx, ctx->symbolPack(), ctx->Symbol());
    return val;
  } break;
  default:
    break;
  }
  return this;
}

std::wstring JSValue::convertToString(common::AutoPtr<JSContext> ctx) {
  switch (getType()) {
  case JSValueType::JS_INTERNAL:
    return L"[internal]";
  case JSValueType::JS_UNDEFINED:
    return L"undefined";
  case JSValueType::JS_NULL:
    return L"null";
  case JSValueType::JS_NUMBER:
    return fmt::format(L"{:g}", getEntity<JSNumberEntity>()->getValue());
  case JSValueType::JS_BIGINT:
    return getEntity<JSBigIntEntity>()->getValue().toString();
  case JSValueType::JS_NAN:
    return L"NaN";
  case JSValueType::JS_INFINITY: {
    auto entity = getEntity<JSInfinityEntity>();
    if (entity->isNegative()) {
      return L"-Infinity";
    } else {
      return L"Infinity";
    }
  }
  case JSValueType::JS_STRING:
    return getEntity<JSStringEntity>()->getValue();
  case JSValueType::JS_BOOLEAN:
    return getEntity<JSBooleanEntity>()->getValue() ? L"true" : L"false";
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_NATIVE_FUNCTION:
  case JSValueType::JS_CLASS:
  case JSValueType::JS_ARGUMENT:
  case JSValueType::JS_ARRAY:
  case JSValueType::JS_FUNCTION:
    return toPrimitive(ctx)->convertToString(ctx);
  case JSValueType::JS_EXCEPTION: {
    auto entity = getEntity<JSExceptionEntity>();
    if (entity->getTarget() != nullptr) {
      return ctx->createValue(entity->getTarget())->convertToString(ctx);
    }
    return entity->toString(ctx);
  }
  case JSValueType::JS_SYMBOL:
    throw error::JSTypeError(L"Cannot convert a Symbol value to a string");

  case JSValueType::JS_UNINITIALIZED:
  case JSValueType::JS_REGEXP:
    break;
  }
  return L"";
}

std::optional<double> JSValue::convertToNumber(common::AutoPtr<JSContext> ctx) {
  switch (getType()) {
  case JSValueType::JS_NULL:
    return 0;
  case JSValueType::JS_NUMBER:
    return getEntity<JSNumberEntity>()->getValue();
  case JSValueType::JS_BOOLEAN:
    return getEntity<JSBooleanEntity>()->getValue() ? 1 : 0;
  case JSValueType::JS_STRING: {
    auto raw = getEntity<JSStringEntity>()->getValue();
    std::wstring snum;
    size_t index = 0;
    enum class TYPE { HEX, OCT, BIN, DEC } type = TYPE::DEC;
    bool negative = false;
    if (raw[index] == L'+' || raw[index] == L'-') {
      if (raw[index] == L'-') {
        negative = true;
      }
      index++;
    }
    if (raw[index] == '0') {
      if (raw[index + 1] == L'x' || raw[index + 1] == L'X') {
        index += 2;
        while ((raw[index] >= '0' && raw[index] <= '9') ||
               (raw[index] >= 'a' && raw[index] <= 'f') ||
               (raw[index] >= 'A' && raw[index] <= 'Z')) {
          snum += raw[index];
          index++;
        }
        if (snum.empty()) {
          snum = L'0';
        } else {
          snum = L"0x" + snum;
          type = TYPE::HEX;
        }
      } else if (raw[index + 1] == L'o' || raw[index + 1] == L'O') {
        index += 2;
        while (raw[index] >= '0' && raw[index] <= '7') {
          snum += raw[index];
          index++;
        }
        if (snum.empty()) {
          snum = L'0';
        } else {
          snum = L"0o" + snum;
          type = TYPE::OCT;
        }
      } else if (raw[index + 1] == L'b' || raw[index + 1] == L'B') {
        index += 2;
        while (raw[index] >= '0' && raw[index] <= '1') {
          snum += raw[index];
          index++;
        }
        if (snum.empty()) {
          snum = L'0';
        } else {
          snum = L"0b" + snum;
          type = TYPE::BIN;
        }
      }
    }
    if (snum.empty()) {
      if (raw[index] == L'.' && raw[index + 1] >= '0' && raw[index + 1] <= 9) {
        snum += L'.';
        index++;
        while (raw[index] >= '0' && raw[index] <= '9') {
          snum += raw[index];
          index++;
        }
      } else if ((raw[index] >= '0' && raw[index] <= '9')) {
        bool dec = false;
        for (;;) {
          if (raw[index] == L'.' && !dec) {
            dec = true;
          } else if (raw[index] < '0' || raw[index] > '9') {
            break;
          }
          snum += raw[index];
          index++;
        }
        bool negative = false;
        if (raw[index] == L'e' || raw[index] == 'E') {
          std::wstring exp;
          index++;
          if (raw[index] == L'+' || raw[index] == L'-') {
            if (raw[index] == L'-') {
              negative = true;
            }
            index++;
          }
          while (raw[index] >= '0' && raw[index] <= '9') {
            exp += raw[index];
            index++;
          }
          if (!exp.empty()) {
            snum += L"e";
            if (negative) {
              snum += L'-';
              snum += exp;
            };
          }
        }
      }
    }
    if (!snum.empty() && negative) {
      snum = L'-' + snum;
    }
    if (!snum.empty() && raw[index] == L'\0') {
      switch (type) {
      case TYPE::HEX:
        return (double)std::stol(snum, nullptr, 16);
      case TYPE::OCT:
        return (double)std::stol(snum, nullptr, 8);
      case TYPE::BIN:
        return (double)std::stol(snum, nullptr, 2);
      case TYPE::DEC:
        return std::stold(snum);
      }
    }
    break;
  }
  case JSValueType::JS_BIGINT:
    throw error::JSTypeError(L"Cannot convert a BigInt value to a number");
  case JSValueType::JS_SYMBOL:
    throw error::JSTypeError(L"Cannot convert a Symbol value to a number");
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_NATIVE_FUNCTION:
  case JSValueType::JS_EXCEPTION:
  case JSValueType::JS_CLASS:
    return toPrimitive(ctx)->convertToNumber(ctx);
  default:
    break;
  }
  return std::nullopt;
}

bool JSValue::convertToBoolean(common::AutoPtr<JSContext> ctx) {
  switch (getType()) {
  case JSValueType::JS_NUMBER:
    return getEntity<JSNumberEntity>()->getValue() != 0;
  case JSValueType::JS_BIGINT:
    return getEntity<JSBigIntEntity>()->getValue() != 0;
  case JSValueType::JS_INFINITY:
    return true;
  case JSValueType::JS_STRING:
    return !getEntity<JSStringEntity>()->getValue().empty();
  case JSValueType::JS_BOOLEAN:
    return getEntity<JSBooleanEntity>()->getValue();
  case JSValueType::JS_SYMBOL:
    return true;
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_NATIVE_FUNCTION:
  case JSValueType::JS_EXCEPTION:
  case JSValueType::JS_CLASS:
    return toPrimitive(ctx)->convertToBoolean(ctx);
  default:
    break;
  }
  return false;
}

common::AutoPtr<JSValue> JSValue::getPrototype(common::AutoPtr<JSContext> ctx) {
  return ctx->createValue(getEntity<JSObjectEntity>()->getPrototype());
}

std::wstring JSValue::getTypeName() {
  switch (getType()) {
  case JSValueType::JS_UNDEFINED:
    return L"undefined";
  case JSValueType::JS_INTERNAL:
  case JSValueType::JS_EXCEPTION:
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_ARGUMENT:
  case JSValueType::JS_NULL:
  case JSValueType::JS_ARRAY:
  case JSValueType::JS_REGEXP:
    return L"object";
  case JSValueType::JS_NAN:
  case JSValueType::JS_INFINITY:
  case JSValueType::JS_NUMBER:
    return L"number";
  case JSValueType::JS_BIGINT:
    return L"bigint";
  case JSValueType::JS_STRING:
    return L"string";
  case JSValueType::JS_BOOLEAN:
    return L"boolean";
  case JSValueType::JS_SYMBOL:
    return L"symbol";
  case JSValueType::JS_CLASS:
  case JSValueType::JS_NATIVE_FUNCTION:
  case JSValueType::JS_FUNCTION:
    return L"function";
  case JSValueType::JS_UNINITIALIZED:
    throw error::JSReferenceError(
        fmt::format(L"Cannot access '{}' before initialization", getName()));
  }
  return L"unknown";
}

JSObjectEntity::JSField *
JSValue::getOwnPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                                  const std::wstring &name) {
  if (getType() == JSValueType::JS_UNDEFINED) {
    throw error::JSTypeError(fmt::format(
        L"Cannot read properties of undefined (reading '{}')", name));
  }
  if (getType() == JSValueType::JS_NULL) {
    throw error::JSTypeError(
        fmt::format(L"Cannot read properties of null (reading '{}')", name));
  }
  auto self = pack(ctx);
  auto &fields = self->getEntity<JSObjectEntity>()->getProperties();
  if (fields.contains(name)) {
    return &fields.at(name);
  }
  return nullptr;
}

JSObjectEntity::JSField *
JSValue::getPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                               const std::wstring &name) {
  if (getType() == JSValueType::JS_UNDEFINED) {
    throw error::JSTypeError(fmt::format(
        L"Cannot read properties of undefined (reading '{}')", name));
  }
  if (getType() == JSValueType::JS_NULL) {
    throw error::JSTypeError(
        fmt::format(L"Cannot read properties of null (reading '{}')", name));
  }
  auto entity = pack(ctx)->getEntity();
  while (entity->getType() >= JSValueType::JS_OBJECT) {
    auto &fields = ((JSObjectEntity *)entity)->getProperties();
    if (fields.contains(name)) {
      return &fields.at(name);
    }
    entity = ((JSObjectEntity *)entity)->getPrototype();
  }
  return nullptr;
}

common::AutoPtr<JSValue>
JSValue::setPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                               const std::wstring &name,
                               const JSObjectEntity::JSField &descriptor) {
  if (descriptor.value && (descriptor.get || descriptor.set)) {
    throw error::JSTypeError(
        fmt::format(L"Invalid property descriptor. Cannot both specify "
                    L"accessors and a value or writable attribute, #<Object>"));
  }
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto old = self->getOwnPropertyDescriptor(ctx, name);
  if (old != nullptr) {
    if (!old->configurable || entity->isFrozen()) {
      throw error::JSTypeError(
          fmt::format(L"Cannot redefine property: '{}'", name));
    }
  } else {
    if (entity->isSealed() || !entity->isExtensible() || entity->isFrozen()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot define property '{}', object is not extensible", name));
    }
  }
  if (old != nullptr) {
    if (old->get != descriptor.get) {
      if (old->get) {
        entity->removeChild(old->get);
        ctx->getScope()->getRoot()->appendChild(old->get);
      }
      if (descriptor.get) {
        entity->appendChild(descriptor.get);
      }
    }
    if (old->set != descriptor.set) {
      if (old->set) {
        entity->removeChild(old->set);
        ctx->getScope()->getRoot()->appendChild(old->set);
      }
      if (descriptor.set) {
        entity->appendChild(descriptor.set);
      }
    }
    if (old->value != descriptor.value) {
      if (old->value) {
        entity->removeChild(old->value);
        ctx->getScope()->getRoot()->appendChild(old->value);
      }
      if (descriptor.value) {
        entity->appendChild(descriptor.value);
      }
    }
  } else {
    if (descriptor.get) {
      entity->appendChild(descriptor.get);
    }
    if (descriptor.set) {
      entity->appendChild(descriptor.set);
    }
    if (descriptor.value) {
      entity->appendChild(descriptor.value);
    }
  }
  entity->getProperties()[name] = descriptor;
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::getProperty(common::AutoPtr<JSContext> ctx,
                                              const std::wstring &name) {
  auto self = pack(ctx);
  auto descriptor = self->getPropertyDescriptor(ctx, name);
  if (descriptor != nullptr) {
    if (descriptor->get) {
      auto getter = ctx->createValue(descriptor->get);
      return getter->apply(ctx, self);
    }
    if (descriptor->value) {
      return ctx->createValue(descriptor->value);
    }
  }
  return ctx->undefined();
}

common::AutoPtr<JSValue>
JSValue::setProperty(common::AutoPtr<JSContext> ctx, const std::wstring &name,
                     const common::AutoPtr<JSValue> &field) {
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto descriptor = self->getOwnPropertyDescriptor(ctx, name);
  if (descriptor && descriptor->value == field->getEntity()) {
    return ctx->truly();
  }
  if (!descriptor) {
    if (entity->isFrozen() || entity->isSealed() || !entity->isExtensible()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot define property '{}', object is not extensible", name));
    }
  } else {
    if (entity->isFrozen()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot assign to read only property '{}' of object '#<Object>'",
          name));
    }
    if (descriptor->value != nullptr &&
        (!descriptor->writable || !descriptor->configurable)) {
      throw error::JSTypeError(fmt::format(
          L"Cannot assign to read only property '{}' of object '#<Object>'",
          name));
    }
    if (descriptor->value == nullptr && !descriptor->set) {
      throw error::JSTypeError(fmt::format(
          L"Cannot set property '{}' of #<Object> which has only a getter",
          name));
    }
  }
  if (descriptor) {
    if (descriptor->value != nullptr) {
      if (descriptor->value != field->getEntity()) {
        entity->removeChild(descriptor->value);
        ctx->getScope()->getRoot()->appendChild(descriptor->value);
        entity->appendChild((JSEntity *)field->getEntity());
        descriptor->value = (JSEntity *)(field->getEntity());
      }
    } else {
      auto setter = ctx->createValue(descriptor->set);
      return setter->apply(ctx, self, {field});
    }
  } else {
    entity->getProperties()[name] = JSObjectEntity::JSField{
        .configurable = true,
        .enumable = true,
        .value = (JSEntity *)field->getEntity(),
        .writable = true,
        .get = nullptr,
        .set = nullptr,
    };
    entity->appendChild((JSEntity *)field->getEntity());
  }
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::removeProperty(common::AutoPtr<JSContext> ctx,
                                                 const std::wstring &name) {
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto descriptor = self->getOwnPropertyDescriptor(ctx, name);
  if (!descriptor) {
    return ctx->truly();
  }
  if (entity->isSealed() || entity->isFrozen()) {
    throw error::JSTypeError(
        fmt::format(L"Cannot delete property '{}' of #<Object>", name));
  }
  if (!descriptor->configurable) {
    throw error::JSTypeError(
        fmt::format(L"Cannot delete property '{}' of #<Object>", name));
  }
  if (descriptor->value) {
    entity->removeChild(descriptor->value);
    ctx->getScope()->getRoot()->appendChild(descriptor->value);
  }
  if (descriptor->get) {
    entity->removeChild(descriptor->get);
    ctx->getScope()->getRoot()->appendChild(descriptor->get);
  }
  if (descriptor->set) {
    entity->removeChild(descriptor->set);
    ctx->getScope()->getRoot()->appendChild(descriptor->set);
  }
  entity->getProperties().erase(name);
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::getIndex(common::AutoPtr<JSContext> ctx,
                                           const uint32_t &index) {
  if (getType() == JSValueType::JS_ARRAY) {
    auto entity = getEntity<JSArrayEntity>();
    auto &items = entity->getItems();
    if (index < items.size()) {
      return ctx->createValue(items[index]);
    }
  }
  return ctx->undefined();
}

common::AutoPtr<JSValue>
JSValue::setIndex(common::AutoPtr<JSContext> ctx, const uint32_t &index,
                  const common::AutoPtr<JSValue> &field) {
  if (getType() == JSValueType::JS_ARRAY) {
    auto entity = getEntity<JSArrayEntity>();
    if (entity->isFrozen() || !entity->isExtensible()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot assign to read only property '{}' of object '{}'", index,
          convertToString(ctx)));
    }
    auto &items = entity->getItems();
    entity->appendChild((JSEntity *)field->getEntity());
    if (index < items.size()) {
      entity->removeChild(items[index]);
    }
    while (items.size() < index) {
      items.push_back(ctx->null()->getEntity());
    }
    if (items.size() == index) {
      items.push_back((JSEntity *)field->getEntity());
    } else {
      items[index] = (JSEntity *)field->getEntity();
    }
    return ctx->truly();
  }
  return setProperty(ctx, fmt::format(L"{}", index), field);
}

JSObjectEntity::JSField *
JSValue::getOwnPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                                  common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return getOwnPropertyDescriptor(ctx, key->convertToString(ctx));
  }
  if (getType() == JSValueType::JS_UNDEFINED) {
    throw error::JSTypeError(
        fmt::format(L"Cannot read properties of undefined (reading '{}')",
                    key->getEntity<JSSymbolEntity>()->getDescription()));
  }
  if (getType() == JSValueType::JS_NULL) {
    throw error::JSTypeError(
        fmt::format(L"Cannot read properties of null (reading '{}')",
                    key->getEntity<JSSymbolEntity>()->getDescription()));
  }
  auto &fields = getEntity<JSObjectEntity>()->getSymbolProperties();
  if (fields.contains(key->getEntity())) {
    return &fields.at(key->getEntity());
  }
  return nullptr;
}

JSObjectEntity::JSField *
JSValue::getPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return getPropertyDescriptor(ctx, key->convertToString(ctx));
  }
  if (getType() == JSValueType::JS_UNDEFINED) {
    throw error::JSTypeError(
        fmt::format(L"Cannot read properties of undefined (reading '{}')",
                    key->getEntity<JSSymbolEntity>()->getDescription()));
  }
  if (getType() == JSValueType::JS_NULL) {
    throw error::JSTypeError(
        fmt::format(L"Cannot read properties of null (reading '{}')",
                    key->getEntity<JSSymbolEntity>()->getDescription()));
  }
  auto self = pack(ctx);
  auto entity = self->getEntity();
  while (entity->getType() >= JSValueType::JS_OBJECT) {
    auto &props = ((JSObjectEntity *)entity)->getSymbolProperties();
    if (props.contains(key->getEntity())) {
      return &props.at(key->getEntity());
    }
    entity = ((JSObjectEntity *)entity)->getPrototype();
  }
  return nullptr;
}

common::AutoPtr<JSValue>
JSValue::setPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> name,
                               const JSObjectEntity::JSField &descriptor) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return setPropertyDescriptor(ctx, key->convertToString(ctx), descriptor);
  }
  if (getType() == JSValueType::JS_UNDEFINED) {
    throw error::JSTypeError(fmt::format(
        L"Cannot read properties of undefined (reading '{}')",
        name->toPrimitive(ctx)->getEntity<JSSymbolEntity>()->getDescription()));
  }
  if (getType() == JSValueType::JS_NULL) {
    throw error::JSTypeError(fmt::format(
        L"Cannot read properties of null (reading '{}')",
        name->toPrimitive(ctx)->getEntity<JSSymbolEntity>()->getDescription()));
  }
  if (descriptor.value && (descriptor.get || descriptor.set)) {
    throw error::JSTypeError(
        fmt::format(L"Invalid property descriptor. Cannot both specify "
                    L"accessors and a value or writable attribute, #<Object>"));
  }
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto old = self->getOwnPropertyDescriptor(ctx, name);
  if (old != nullptr) {
    if (!old->configurable || entity->isFrozen()) {
      throw error::JSTypeError(
          fmt::format(L"Cannot redefine property: 'Symbol({})'",
                      key->getEntity<JSSymbolEntity>()->getDescription()));
    }
  } else {
    if (entity->isSealed() || !entity->isExtensible() || entity->isFrozen()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot define property 'Symbol({})', object is not extensible",
          key->getEntity<JSSymbolEntity>()->getDescription()));
    }
  }
  if (old != nullptr) {
    if (old->get != descriptor.get) {
      if (old->get) {
        entity->removeChild(old->get);
        ctx->getScope()->getRoot()->appendChild(old->get);
      }
      if (descriptor.get) {
        entity->appendChild(descriptor.get);
      }
    }
    if (old->set != descriptor.set) {
      if (old->set) {
        entity->removeChild(old->set);
        ctx->getScope()->getRoot()->appendChild(old->set);
      }
      if (descriptor.set) {
        entity->appendChild(descriptor.set);
      }
    }
    if (old->value != descriptor.value) {
      if (old->value) {
        entity->removeChild(old->value);
        ctx->getScope()->getRoot()->appendChild(old->value);
      }
      if (descriptor.value) {
        entity->appendChild(descriptor.value);
      }
    }
  } else {
    if (descriptor.get) {
      entity->appendChild(descriptor.get);
    }
    if (descriptor.set) {
      entity->appendChild(descriptor.set);
    }
    if (descriptor.value) {
      entity->appendChild(descriptor.value);
    }
  }
  entity->getSymbolProperties()[key->getEntity()] = descriptor;
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::getProperty(common::AutoPtr<JSContext> ctx,
                                              common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    if (getType() == JSValueType::JS_ARRAY) {
      auto num = key->convertToNumber(ctx);
      if (num.has_value()) {
        return getIndex(ctx, num.value());
      }
    }
    return getProperty(ctx, key->convertToString(ctx));
  }
  auto self = pack(ctx);
  auto descriptor = self->getPropertyDescriptor(ctx, name);
  if (descriptor != nullptr) {
    if (descriptor->get) {
      auto getter = ctx->createValue(descriptor->get);
      return getter->apply(ctx, self);
    }
    if (descriptor->value) {
      return ctx->createValue(descriptor->value);
    }
  }
  return ctx->undefined();
}

common::AutoPtr<JSValue>
JSValue::setProperty(common::AutoPtr<JSContext> ctx,
                     common::AutoPtr<JSValue> name,
                     const common::AutoPtr<JSValue> &field) {
  auto key = name->toPrimitive(ctx);
  if (getType() == JSValueType::JS_ARRAY) {
    auto num = key->convertToNumber(ctx);
    if (num.has_value()) {
      return setIndex(ctx, num.value(), field);
    }
  }
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return setProperty(ctx, key->convertToString(ctx), field);
  }
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto descriptor = self->getOwnPropertyDescriptor(ctx, name);
  if (descriptor && descriptor->value == field->getEntity()) {
    return ctx->truly();
  }
  if (!descriptor) {
    if (entity->isFrozen() || entity->isSealed() || !entity->isExtensible()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot define property 'Symbol({})', object is not extensible",
          key->getEntity<JSSymbolEntity>()->getDescription()));
    }
  } else {
    if (entity->isFrozen()) {
      throw error::JSTypeError(
          fmt::format(L"Cannot assign to read only property 'Symbol({})' of "
                      L"object '#<Object>'",
                      key->getEntity<JSSymbolEntity>()->getDescription()));
    }
    if (descriptor->value != nullptr &&
        (!descriptor->writable || !descriptor->configurable)) {
      throw error::JSTypeError(
          fmt::format(L"Cannot assign to read only property 'Symbol({})' of "
                      L"object '#<Object>'",
                      key->getEntity<JSSymbolEntity>()->getDescription()));
    }
    if (descriptor->value == nullptr && !descriptor->set) {
      throw error::JSTypeError(
          fmt::format(L"Cannot set property 'Symbol({})' of #<Object> which "
                      L"has only a getter",
                      key->getEntity<JSSymbolEntity>()->getDescription()));
    }
  }
  if (descriptor) {
    if (descriptor->value != nullptr) {
      if (descriptor->value != field->getEntity()) {
        entity->removeChild(descriptor->value);
        ctx->getScope()->getRoot()->appendChild(descriptor->value);
        entity->appendChild((JSEntity *)field->getEntity());
        descriptor->value = (JSEntity *)(field->getEntity());
      }
    } else {
      auto setter = ctx->createValue(descriptor->set);
      return setter->apply(ctx, self, {field});
    }
  } else {
    entity->getSymbolProperties()[key->getEntity()] = JSObjectEntity::JSField{
        .configurable = true,
        .enumable = true,
        .value = (JSEntity *)field->getEntity(),
        .writable = true,
        .get = nullptr,
        .set = nullptr,
    };
    entity->appendChild((JSEntity *)field->getEntity());
  }
  return ctx->truly();
}

common::AutoPtr<JSValue>
JSValue::removeProperty(common::AutoPtr<JSContext> ctx,
                        common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return removeProperty(ctx, key->convertToString(ctx));
  }
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto descriptor = self->getOwnPropertyDescriptor(ctx, name);
  if (!descriptor) {
    return ctx->truly();
  }
  if (entity->isSealed() || entity->isFrozen()) {
    throw error::JSTypeError(
        fmt::format(L"Cannot delete property 'Symbol({})' of #<Object>",
                    key->getEntity<JSSymbolEntity>()->getDescription()));
  }
  if (!descriptor->configurable) {
    throw error::JSTypeError(
        fmt::format(L"Cannot delete property 'Symbol({})' of #<Object>",
                    key->getEntity<JSSymbolEntity>()->getDescription()));
  }
  if (descriptor->value) {
    entity->removeChild(descriptor->value);
    ctx->getScope()->getRoot()->appendChild(descriptor->value);
  }
  if (descriptor->get) {
    entity->removeChild(descriptor->get);
    ctx->getScope()->getRoot()->appendChild(descriptor->get);
  }
  if (descriptor->set) {
    entity->removeChild(descriptor->set);
    ctx->getScope()->getRoot()->appendChild(descriptor->set);
  }
  entity->getSymbolProperties().erase(key->getEntity());
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::unaryPlus(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(((JSInfinityEntity *)_entity)->isNegative());
  }
  auto value = convertToNumber(ctx);
  if (value.has_value()) {
    return ctx->createNumber(value.value());
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue>
JSValue::unaryNetation(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(!((JSInfinityEntity *)_entity)->isNegative());
  }
  if (getType() == JSValueType::JS_BIGINT) {
    return ctx->createBigInt(-((JSBigIntEntity *)_entity)->getValue());
  }
  auto value = convertToNumber(ctx);
  if (value.has_value()) {
    return ctx->createNumber(-value.value());
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::increment(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(((JSInfinityEntity *)_entity)->isNegative());
  }
  if (getType() == JSValueType::JS_BIGINT) {
    auto &value = ((JSBigIntEntity *)_entity)->getValue();
    value += 1;
    return ctx->createBigInt(value);
  }
  auto value = convertToNumber(ctx);
  if (value.has_value()) {
    setNumber(value.value() + 1);
    return ctx->createNumber(value.value() + 1);
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::decrement(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(((JSInfinityEntity *)_entity)->isNegative());
  }
  if (getType() == JSValueType::JS_BIGINT) {
    auto &value = ((JSBigIntEntity *)_entity)->getValue();
    value -= 1;
    return ctx->createBigInt(value);
  }
  auto value = convertToNumber(ctx);
  if (value.has_value()) {
    setNumber(value.value() - 1);
    return ctx->createNumber(value.value() - 1);
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::logicalNot(common::AutoPtr<JSContext> ctx) {
  return ctx->createBoolean(!convertToBoolean(ctx));
}

common::AutoPtr<JSValue> JSValue::bitwiseNot(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_BIGINT) {
    auto value = ((JSBigIntEntity *)_entity)->getValue();
    return ctx->createBigInt(~value);
  }
  auto num = convertToNumber(ctx);
  if (num.has_value()) {
    return ctx->createNumber(~(int64_t)num.value());
  }
  return ctx->createNumber(~0);
}

common::AutoPtr<JSValue> JSValue::add(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_STRING ||
      right->getType() == JSValueType::JS_STRING) {
    return ctx->createString(left->convertToString(ctx) +
                             right->convertToString(ctx));
  }
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() +
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->convertToNumber(ctx);
  auto rightval = right->convertToNumber(ctx);
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createNumber(leftval.value() + rightval.value());
  }
  if (left->isInfinity() && right->isInfinity()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative() !=
        right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->NaN();
    } else {
      return ctx->createInfinity(
          left->getEntity<JSInfinityEntity>()->isNegative());
    }
  }
  if (left->isInfinity()) {
    return ctx->createValue(left);
  }
  if (right->isInfinity()) {
    return ctx->createValue(right);
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::sub(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = toPrimitive(ctx);
  auto leftval = left->convertToNumber(ctx);
  auto rightval = right->convertToNumber(ctx);
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createNumber(leftval.value() - rightval.value());
  }
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() -
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  if (left->isInfinity() && right->isInfinity()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative() ==
        right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->NaN();
    }
  }
  if (left->isInfinity()) {
    return ctx->createValue(left);
  }
  if (right->isInfinity()) {
    return ctx->createInfinity(
        !right->getEntity<JSInfinityEntity>()->isNegative());
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::mul(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = toPrimitive(ctx);
  auto leftval = left->convertToNumber(ctx);
  auto rightval = right->convertToNumber(ctx);
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createNumber(leftval.value() * rightval.value());
  }
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() *
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  if (left->getType() == JSValueType::JS_INFINITY &&
      right->getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(
        left->getEntity<JSInfinityEntity>()->isNegative() ==
        right->getEntity<JSInfinityEntity>()->isNegative());
  }
  if (left->isInfinity()) {
    if (rightval.has_value()) {
      return ctx->createInfinity(rightval.value() < 0);
    }
  }
  if (right->isInfinity()) {
    if (leftval.has_value()) {
      return ctx->createInfinity(leftval.value() < 0);
    }
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::div(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = toPrimitive(ctx);
  auto leftval = left->convertToNumber(ctx);
  auto rightval = right->convertToNumber(ctx);
  if (leftval.has_value() && rightval.has_value()) {
    if (rightval.value() == 0) {
      return ctx->createInfinity(leftval.value() < 0);
    }
    return ctx->createNumber(leftval.value() / rightval.value());
  }
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() /
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  if (left->isInfinity() && right->isInfinity()) {
    return ctx->NaN();
  }
  if (left->isInfinity()) {
    if (rightval.has_value()) {
      return ctx->createInfinity(rightval.value() < 0);
    }
  }
  if (right->isInfinity()) {
    if (rightval.has_value()) {
      return ctx->createNumber();
    }
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::mod(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = toPrimitive(ctx);
  auto leftval = left->convertToNumber(ctx);
  auto rightval = right->convertToNumber(ctx);
  if (leftval.has_value() && rightval.has_value()) {
    if (rightval.value() == 0) {
      return ctx->NaN();
    }
    return ctx->createNumber(leftval.value() -
                             ((int64_t)(leftval.value() / rightval.value())) *
                                 rightval.value());
  }
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() %
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  if (left->isInfinity()) {
    return ctx->NaN();
  }
  if (right->isInfinity()) {
    if (leftval.has_value()) {
      return ctx->createNumber(leftval.value());
    }
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue> JSValue::equal(common::AutoPtr<JSContext> ctx,
                                        common::AutoPtr<JSValue> another) {
  if (getType() == another->getType()) {
    switch (getType()) {
    case JSValueType::JS_NUMBER:
      return ctx->createBoolean(getNumber().value() ==
                                another->getNumber().value());
    case JSValueType::JS_BIGINT:
      return ctx->createBoolean(getBigInt().value() ==
                                another->getBigInt().value());
    case JSValueType::JS_NAN:
      return ctx->falsely();
    case JSValueType::JS_STRING:
      return ctx->createBoolean(getString().value() ==
                                another->getString().value());
    case JSValueType::JS_BOOLEAN:
      return ctx->createBoolean(getBoolean().value() ==
                                another->getBoolean().value());
    case JSValueType::JS_SYMBOL:
    case JSValueType::JS_OBJECT:
    case JSValueType::JS_NATIVE_FUNCTION:
    case JSValueType::JS_EXCEPTION:
    case JSValueType::JS_CLASS:
      return ctx->createBoolean(getEntity() == another->getEntity());
    default:
      break;
    }
    return ctx->truly();
  }
  if (isUndefined() || isNull()) {
    return ctx->createBoolean(another->isUndefined() || another->isNull());
  }
  if (getType() >= JSValueType::JS_OBJECT &&
      another->getType() >= JSValueType::JS_OBJECT) {
    return ctx->falsely();
  }
  if (getType() >= JSValueType::JS_OBJECT) {
    return toPrimitive(ctx)->equal(ctx, another);
  }
  if (another->getType() >= JSValueType::JS_OBJECT) {
    return equal(ctx, another->toPrimitive(ctx));
  }
  if (getType() == JSValueType::JS_BOOLEAN) {
    return ctx->createNumber(convertToNumber(ctx).value())->equal(ctx, another);
  }
  if (another->getType() == JSValueType::JS_BOOLEAN) {
    return equal(ctx, ctx->createNumber(another->convertToNumber(ctx).value()));
  }
  if (getType() == JSValueType::JS_NUMBER &&
      another->getType() == JSValueType::JS_STRING) {
    auto num = another->convertToNumber(ctx);
    if (num.has_value()) {
      return ctx->createBoolean(getNumber().value() == num.value());
    }
  }
  if (getType() == JSValueType::JS_STRING &&
      another->getType() == JSValueType::JS_NUMBER) {
    auto num = convertToNumber(ctx);
    if (num.has_value()) {
      return ctx->createBoolean(num.value() == another->getNumber().value());
    }
  }
  if (getType() == JSValueType::JS_NUMBER &&
      another->getType() == JSValueType::JS_BIGINT) {
    return ctx->createBoolean(common::BigInt<>(getNumber().value()) ==
                              another->getBigInt());
  }
  if (getType() == JSValueType::JS_BIGINT &&
      another->getType() == JSValueType::JS_NUMBER) {
    return ctx->createBoolean(another->getBigInt() ==
                              common::BigInt<>(getNumber().value()));
  }
  if (getType() == JSValueType::JS_STRING &&
      another->getType() == JSValueType::JS_BIGINT) {
    try {
      return ctx->createBoolean(common::BigInt<>(getString().value()) ==
                                another->getBigInt());
    } catch (...) {
      return ctx->falsely();
    }
  }
  if (getType() == JSValueType::JS_BIGINT &&
      another->getType() == JSValueType::JS_STRING) {
    try {
      return ctx->createBoolean(another->getBigInt() ==
                                common::BigInt<>(getString().value()));
    } catch (...) {
      return ctx->falsely();
    }
  }
  return ctx->falsely();
}

common::AutoPtr<JSValue> JSValue::notEqual(common::AutoPtr<JSContext> ctx,
                                           common::AutoPtr<JSValue> another) {
  return ctx->createBoolean(!equal(ctx, another)->getBoolean().value());
}

common::AutoPtr<JSValue>
JSValue::strictEqual(common::AutoPtr<JSContext> ctx,
                     common::AutoPtr<JSValue> another) {
  if (getType() == another->getType()) {
    switch (getType()) {
    case JSValueType::JS_NUMBER:
      return ctx->createBoolean(getNumber().value() ==
                                another->getNumber().value());
    case JSValueType::JS_BIGINT:
      return ctx->createBoolean(getBigInt().value() ==
                                another->getBigInt().value());
    case JSValueType::JS_NAN:
      return ctx->falsely();
    case JSValueType::JS_STRING:
      return ctx->createBoolean(getString().value() ==
                                another->getString().value());
    case JSValueType::JS_BOOLEAN:
      return ctx->createBoolean(getBoolean().value() ==
                                another->getBoolean().value());
    case JSValueType::JS_SYMBOL:
    case JSValueType::JS_OBJECT:
    case JSValueType::JS_NATIVE_FUNCTION:
    case JSValueType::JS_EXCEPTION:
    case JSValueType::JS_CLASS:
      return ctx->createBoolean(getEntity() == another->getEntity());
    default:
      break;
    }
    return ctx->truly();
  }
  return ctx->falsely();
}

common::AutoPtr<JSValue>
JSValue::notStrictEqual(common::AutoPtr<JSContext> ctx,
                        common::AutoPtr<JSValue> another) {
  return ctx->createBoolean(!strictEqual(ctx, another)->getBoolean().value());
}