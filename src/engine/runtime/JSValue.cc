#include "engine/runtime/JSValue.hpp"
#include "common/AutoPtr.hpp"
#include "common/BigInt.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSArrayEntity.hpp"
#include "engine/entity/JSBigIntEntity.hpp"
#include "engine/entity/JSBooleanEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/entity/JSNaNEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/entity/JSNumberEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSStore.hpp"
#include "error/JSError.hpp"
#include "error/JSInternalError.hpp"
#include "error/JSRangeError.hpp"
#include "error/JSReferenceError.hpp"
#include "error/JSSyntaxError.hpp"
#include "error/JSTypeError.hpp"
#include <cmath>
#include <codecvt>
#include <exception>
#include <fmt/xchar.h>
#include <locale>
#include <string>

using namespace spark;
using namespace spark::engine;
JSValue::JSValue(JSScope *scope, JSStore *store)
    : _scope(scope), _store(store), _const(false) {}

JSValue::~JSValue() {}

const JSValueType &JSValue::getType() const {
  if (!_store) {
    throw error::JSInternalError(L"out of scope");
  }
  return getEntity()->getType();
}

std::wstring JSValue::getName() const {
  for (auto &[name, val] : _scope->getValues()) {
    if (val == this) {
      return name;
    }
  }
  return L"[[anonymous]]";
}

void JSValue::setConst() { _const = true; }

bool JSValue::isConst() const { return _const; };

JSStore *JSValue::getStore() { return _store; }

const JSStore *JSValue::getStore() const { return _store; }

common::AutoPtr<JSScope> JSValue::getScope() { return _scope; }

void JSValue::setEntity(const common::AutoPtr<JSEntity> &entity) {
  _store->setEntity(entity);
}
void JSValue::setStore(JSStore *store) {
  if (_store != store) {
    if (_const && getType() != JSValueType::JS_UNINITIALIZED) {
      throw error::JSTypeError(L"Assignment to constant variable.");
    }
    _store->setEntity(store->getEntity());
    for (auto &child : _store->getChildren()) {
      _store->removeChild(child);
    }
    for (auto &parent : store->getParent()) {
      parent->appendChild(_store);
    }
    for (auto &child : store->getChildren()) {
      _store->appendChild(child);
    }
  }
}

std::optional<double> JSValue::getNumber() const {
  if (getType() == JSValueType::JS_NUMBER) {
    return getEntity<JSNumberEntity>()->getValue();
  }
  return std::nullopt;
}

std::optional<std::wstring> JSValue::getString() const {
  if (getType() == JSValueType::JS_STRING) {
    return getEntity<JSStringEntity>()->getValue();
  }
  return std::nullopt;
}

std::optional<bool> JSValue::getBoolean() const {
  if (getType() == JSValueType::JS_BOOLEAN) {
    return getEntity<JSBooleanEntity>()->getValue();
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

bool JSValue::isFunction() const {
  return getType() == JSValueType::JS_FUNCTION ||
         getType() == JSValueType::JS_NATIVE_FUNCTION;
}
bool JSValue::isObject() const { return getType() == JSValueType::JS_OBJECT; }

bool JSValue::isException() const {
  return getType() == JSValueType::JS_EXCEPTION;
}

void JSValue::setNumber(double value) {
  if (getType() != JSValueType::JS_NUMBER) {
    _store->setEntity(new JSNumberEntity(value));
  } else {
    getEntity<JSNumberEntity>()->getValue() = value;
  }
}

void JSValue::setString(const std::wstring &value) {
  if (getType() != JSValueType::JS_STRING) {
    _store->setEntity(new JSStringEntity(value));
  } else {
    getEntity<JSStringEntity>()->getValue() = value;
  }
}

void JSValue::setBoolean(bool value) {
  if (getType() != JSValueType::JS_BOOLEAN) {
    _store->setEntity(new JSBooleanEntity(value));
  } else {
    getEntity<JSBooleanEntity>()->getValue() = value;
  }
}

common::AutoPtr<JSValue>
JSValue::apply(common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> self,
               std::vector<common::AutoPtr<JSValue>> args,
               const JSLocation &location) {
  common::AutoPtr<JSValue> func = this;
  if (getType() == JSValueType::JS_OBJECT) {
    func = toPrimitive(ctx);
  }
  if (func->getType() != JSValueType::JS_NATIVE_FUNCTION &&
      func->getType() != JSValueType::JS_FUNCTION) {

    throw error::JSTypeError(
        fmt::format(L"'{}' is not a function, value is '{}'", getName(),
                    toString(ctx)->getString().value()),
        location);
  }
  if (!location.funcname.empty() || location.filename != 0 ||
      location.column != 0 || location.line != 0) {
    ctx->pushCallStack(location);
  }
  auto scope = ctx->getScope();
  ctx->pushScope();
  JSStore *store = nullptr;
  try {
    auto vm = ctx->getRuntime()->getVirtualMachine();
    store = vm->apply(ctx, func, self, args)->getStore();
  } catch (error::JSError &e) {
    store =
        ctx->createException(e.getType(), e.getMessage(), location)->getStore();
  } catch (std::exception &e) {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    store = ctx->createException(L"InternalError",
                                 converter.from_bytes(e.what()), location)
                ->getStore();
  }
  scope->getRoot()->appendChild(store);
  while (ctx->getScope() != scope) {
    ctx->popScope();
  }
  if (!location.funcname.empty() || location.filename != 0 ||
      location.column != 0 || location.line != 0) {
    ctx->popCallStack();
  }
  return ctx->createValue(store);
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
  case JSValueType::JS_NULL:
  case JSValueType::JS_UNDEFINED:
    throw error::JSTypeError(L"Cannot convert undefined or null to object");
  default:
    break;
  }
  return this;
}

common::AutoPtr<JSValue> JSValue::toNumber(common::AutoPtr<JSContext> ctx) {
  double value = .0f;
  switch (getType()) {
  case JSValueType::JS_UNDEFINED:
  case JSValueType::JS_NULL:
    break;
  case JSValueType::JS_NAN:
  case JSValueType::JS_INFINITY:
  case JSValueType::JS_NUMBER:
    return this;
  case JSValueType::JS_BOOLEAN:
    value = getEntity<JSBooleanEntity>()->getValue() ? 1 : 0;
    break;
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
      if (raw[index] == L'.' && raw[index + 1] >= '0' &&
          raw[index + 1] <= '9') {
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
        value = (double)std::stol(snum, nullptr, 16);
        break;
      case TYPE::OCT:
        value = (double)std::stol(snum, nullptr, 8);
        break;
      case TYPE::BIN:
        value = (double)std::stol(snum, nullptr, 2);
        break;
      case TYPE::DEC:
        value = std::stold(snum);
        break;
      }
      return ctx->createNumber(value);
    } else {
      return ctx->NaN();
    }
  }
  case JSValueType::JS_BIGINT:
    throw error::JSTypeError(L"Cannot convert a BigInt value to a number");
  case JSValueType::JS_SYMBOL:
    throw error::JSTypeError(L"Cannot convert a Symbol value to a number");
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_CLASS:
  case JSValueType::JS_ARRAY:
  case JSValueType::JS_FUNCTION:
  case JSValueType::JS_NATIVE_FUNCTION:
    return toPrimitive(ctx)->toNumber(ctx);
  case JSValueType::JS_UNINITIALIZED:
    throw error::JSSyntaxError(
        fmt::format(L"'{}' is uninitialized", getName()));
  case JSValueType::JS_INTERNAL:
  case JSValueType::JS_TASK:
  case JSValueType::JS_EXCEPTION:
    throw error::JSSyntaxError(L"interanl variable cannot convert to number");
    break;
  case JSValueType::JS_REGEXP:
    throw error::JSSyntaxError(L"not implement");
  }
  return ctx->createNumber(value);
}

common::AutoPtr<JSValue> JSValue::toBoolean(common::AutoPtr<JSContext> ctx) {
  bool value = false;
  switch (getType()) {
  case JSValueType::JS_NUMBER:
    value = getEntity<JSNumberEntity>()->getValue() != 0;
    break;
  case JSValueType::JS_BIGINT:
    value = getEntity<JSBigIntEntity>()->getValue() != 0;
    break;
  case JSValueType::JS_INFINITY:
    value = true;
    break;
  case JSValueType::JS_STRING:
    value = !getEntity<JSStringEntity>()->getValue().empty();
    break;
  case JSValueType::JS_BOOLEAN:
    value = getEntity<JSBooleanEntity>()->getValue();
    break;
  case JSValueType::JS_SYMBOL:
    value = true;
    break;
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_NATIVE_FUNCTION:
  case JSValueType::JS_EXCEPTION:
  case JSValueType::JS_CLASS:
    return toPrimitive(ctx)->toBoolean(ctx);
  default:
    break;
  }
  return ctx->createBoolean(value);
}

common::AutoPtr<JSValue> JSValue::toString(common::AutoPtr<JSContext> ctx) {
  std::wstring str;
  switch (getType()) {
  case JSValueType::JS_INTERNAL:
    str = L"[internal]";
    break;
  case JSValueType::JS_TASK:
    str = L"[task]";
    break;
  case JSValueType::JS_UNDEFINED:
    str = L"undefined";
    break;
  case JSValueType::JS_NULL:
    str = L"null";
    break;
  case JSValueType::JS_NUMBER:
    str = fmt::format(L"{:g}", getEntity<JSNumberEntity>()->getValue());
    break;
  case JSValueType::JS_BIGINT:
    str = getEntity<JSBigIntEntity>()->getValue().toString();
    break;
  case JSValueType::JS_NAN:
    str = L"NaN";
    break;
  case JSValueType::JS_INFINITY: {
    auto entity = getEntity<JSInfinityEntity>();
    if (entity->isNegative()) {
      str = L"-Infinity";
    } else {
      str = L"Infinity";
    }
    break;
  }
  case JSValueType::JS_STRING:
    str = getEntity<JSStringEntity>()->getValue();
    break;
  case JSValueType::JS_BOOLEAN:
    str = getEntity<JSBooleanEntity>()->getValue() ? L"true" : L"false";
    break;
  case JSValueType::JS_OBJECT:
  case JSValueType::JS_NATIVE_FUNCTION:
  case JSValueType::JS_CLASS:
  case JSValueType::JS_ARRAY:
  case JSValueType::JS_FUNCTION:
    return toPrimitive(ctx)->toString(ctx);
  case JSValueType::JS_EXCEPTION: {
    auto entity = getEntity<JSExceptionEntity>();
    if (entity->getTarget() != nullptr) {
      return ctx->createValue(entity->getTarget())->toString(ctx);
    }
    str = entity->toString(ctx);
    break;
  }
  case JSValueType::JS_SYMBOL:
    throw error::JSTypeError(L"Cannot convert a Symbol value to a string");

  case JSValueType::JS_UNINITIALIZED:
    throw error::JSSyntaxError(
        fmt::format(L"'{}' is uninitialized", getName()));
  case JSValueType::JS_REGEXP:
    throw error::JSTypeError(L"Not implement");
  }
  return ctx->createString(str);
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
  case JSValueType::JS_TASK:
    break;
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
  auto entity = pack(ctx)->getEntity<JSObjectEntity>();
  while (entity != nullptr) {
    auto &fields = entity->getProperties();
    if (fields.contains(name)) {
      return &fields.at(name);
    }
    entity = entity->getPrototype()->getEntity().cast<JSObjectEntity>();
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
  auto store = self->getStore();
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
        store->removeChild(old->get);
        ctx->getScope()->getRoot()->appendChild(old->get);
      }
      if (descriptor.get) {
        store->appendChild(descriptor.get);
      }
    }
    if (old->set != descriptor.set) {
      if (old->set) {
        store->removeChild(old->set);
        ctx->getScope()->getRoot()->appendChild(old->set);
      }
      if (descriptor.set) {
        store->appendChild(descriptor.set);
      }
    }
    if (old->value != descriptor.value) {
      if (old->value) {
        store->removeChild(old->value);
        ctx->getScope()->getRoot()->appendChild(old->value);
      }
      if (descriptor.value) {
        store->appendChild(descriptor.value);
      }
    }
  } else {
    if (descriptor.get) {
      store->appendChild(descriptor.get);
    }
    if (descriptor.set) {
      store->appendChild(descriptor.set);
    }
    if (descriptor.value) {
      store->appendChild(descriptor.value);
    }
  }
  entity->getProperties()[name] = descriptor;
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::setPropertyDescriptor(
    common::AutoPtr<JSContext> ctx, const std::wstring &name,
    const common::AutoPtr<JSValue> &value, bool configurable, bool enumable,
    bool writable) {
  return setPropertyDescriptor(ctx, name,
                               JSObjectEntity::JSField{
                                   .configurable = configurable,
                                   .enumable = enumable,
                                   .value = (JSStore *)value->getStore(),
                                   .writable = writable,
                                   .get = nullptr,
                                   .set = nullptr,
                               });
}

common::AutoPtr<JSValue> JSValue::setPropertyDescriptor(
    common::AutoPtr<JSContext> ctx, const std::wstring &name,
    const common::AutoPtr<JSValue> &get, const common::AutoPtr<JSValue> &set,
    bool configurable, bool enumable) {
  return setPropertyDescriptor(
      ctx, name,
      JSObjectEntity::JSField{
          .configurable = configurable,
          .enumable = enumable,
          .value = nullptr,
          .writable = false,
          .get = get != nullptr ? (JSStore *)get->getStore() : nullptr,
          .set = set != nullptr ? (JSStore *)set->getStore() : nullptr,
      });
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
  auto store = self->getStore();
  auto entity = self->getEntity<JSObjectEntity>();
  auto descriptor = self->getOwnPropertyDescriptor(ctx, name);
  if (descriptor && descriptor->value == field->getStore()) {
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
      if (descriptor->value != field->getStore()) {
        store->removeChild(descriptor->value);
        ctx->getScope()->getRoot()->appendChild(descriptor->value);
        store->appendChild((JSStore *)field->getStore());
        descriptor->value = (JSStore *)(field->getStore());
      }
    } else {
      auto setter = ctx->createValue(descriptor->set);
      return setter->apply(ctx, self, {field});
    }
  } else {
    entity->getProperties()[name] = JSObjectEntity::JSField{
        .configurable = true,
        .enumable = true,
        .value = (JSStore *)field->getStore(),
        .writable = true,
        .get = nullptr,
        .set = nullptr,
    };
    store->appendChild((JSStore *)field->getStore());
  }
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::removeProperty(common::AutoPtr<JSContext> ctx,
                                                 const std::wstring &name) {
  auto self = pack(ctx);
  auto store = self->getStore();
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
    store->removeChild(descriptor->value);
    ctx->getScope()->getRoot()->appendChild(descriptor->value);
  }
  if (descriptor->get) {
    store->removeChild(descriptor->get);
    ctx->getScope()->getRoot()->appendChild(descriptor->get);
  }
  if (descriptor->set) {
    store->removeChild(descriptor->set);
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
      auto item = items[index];
      if (!item) {
        return ctx->undefined();
      }
      if (item->getEntity()->getType() == JSValueType::JS_UNINITIALIZED) {
        return ctx->undefined();
      }
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
    auto store = getStore();
    if (entity->isFrozen() || !entity->isExtensible()) {
      throw error::JSTypeError(fmt::format(
          L"Cannot assign to read only property '{}' of object '{}'", index,
          toString(ctx)->getString().value()));
    }
    auto &items = entity->getItems();
    store->appendChild((JSStore *)field->getStore());
    if (index < items.size()) {
      store->removeChild(items[index]);
      ctx->getScope()->getRoot()->appendChild(items[index]);
    }
    while (items.size() < index) {
      items.resize(index);
    }
    if (items.size() == index) {
      items.push_back((JSStore *)field->getStore());
    } else {
      items[index] = (JSStore *)field->getStore();
    }
    store->appendChild((JSStore *)field->getStore());
    return ctx->truly();
  }
  return setProperty(ctx, fmt::format(L"{}", index), field);
}

common::AutoPtr<JSValue> JSValue::getKeys(common::AutoPtr<JSContext> ctx) {
  auto result = ctx->createArray();
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  uint32_t index = 0;
  while (entity != nullptr) {
    if (self->getType() == JSValueType::JS_ARRAY) {
      auto &items = entity.cast<JSArrayEntity>()->getItems();
      for (size_t offset = 0; offset < items.size(); offset++) {
        if (items[offset] != nullptr) {
          result->setIndex(ctx, index++,
                           ctx->createString(fmt::format(L"{}", offset)));
        }
      }
    }
    auto &props = entity->getProperties();
    for (auto &[key, field] : props) {
      if (field.enumable) {
        result->setIndex(ctx, index++, ctx->createString(key));
      }
    }
    self = self->getPrototype(ctx);
    entity = self->getEntity<JSObjectEntity>();
  }
  return result;
}

common::AutoPtr<JSValue> JSValue::getBind(common::AutoPtr<JSContext> ctx) {
  if (!isFunction()) {
    throw error::JSTypeError(
        fmt::format(L"cannot convert '{}' to function", getTypeName()));
  }
  if (getType() == JSValueType::JS_FUNCTION) {
    auto entity = getEntity<JSFunctionEntity>();
    auto bind = entity->getBind();
    if (bind != nullptr) {
      return ctx->createValue(bind);
    }
  }
  if (getType() == JSValueType::JS_NATIVE_FUNCTION) {
    auto entity = getEntity<JSNativeFunctionEntity>();
    auto bind = entity->getBind();
    if (bind != nullptr) {
      return ctx->createValue(bind);
    }
  }
  return nullptr;
}

void JSValue::setBind(common::AutoPtr<JSContext> ctx,
                      common::AutoPtr<JSValue> bind) {
  if (isFunction()) {
    _store->appendChild(bind->getStore());
    if (getType() == JSValueType::JS_FUNCTION) {
      auto current = getEntity<JSFunctionEntity>()->getBind();
      if (current) {
        return;
      }
      getEntity<JSFunctionEntity>()->bind(bind->getStore());
    } else {
      auto current = getEntity<JSNativeFunctionEntity>()->getBind();
      if (current) {
        return;
      }
      getEntity<JSNativeFunctionEntity>()->bind(bind->getStore());
    }
  }
}

JSObjectEntity::JSField *
JSValue::getOwnPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                                  common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return getOwnPropertyDescriptor(ctx,
                                    key->toString(ctx)->getString().value());
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
  if (fields.contains(key->getStore())) {
    return &fields.at(key->getStore());
  }
  return nullptr;
}

JSObjectEntity::JSField *
JSValue::getPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return getPropertyDescriptor(ctx, key->toString(ctx)->getString().value());
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
  auto entity = self->getEntity<JSObjectEntity>();
  while (entity != nullptr) {
    auto &props = entity->getSymbolProperties();
    if (props.contains(key->getStore())) {
      return &props.at(key->getStore());
    }
    entity = entity->getPrototype()->getEntity().cast<JSObjectEntity>();
  }
  return nullptr;
}

common::AutoPtr<JSValue>
JSValue::setPropertyDescriptor(common::AutoPtr<JSContext> ctx,
                               common::AutoPtr<JSValue> name,
                               const JSObjectEntity::JSField &descriptor) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return setPropertyDescriptor(ctx, key->toString(ctx)->getString().value(),
                                 descriptor);
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
  auto store = self->getStore();
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
        store->removeChild(old->get);
        ctx->getScope()->getRoot()->appendChild(old->get);
      }
      if (descriptor.get) {
        store->appendChild(descriptor.get);
      }
    }
    if (old->set != descriptor.set) {
      if (old->set) {
        store->removeChild(old->set);
        ctx->getScope()->getRoot()->appendChild(old->set);
      }
      if (descriptor.set) {
        store->appendChild(descriptor.set);
      }
    }
    if (old->value != descriptor.value) {
      if (old->value) {
        store->removeChild(old->value);
        ctx->getScope()->getRoot()->appendChild(old->value);
      }
      if (descriptor.value) {
        store->appendChild(descriptor.value);
      }
    }
  } else {
    if (descriptor.get) {
      store->appendChild(descriptor.get);
    }
    if (descriptor.set) {
      store->appendChild(descriptor.set);
    }
    if (descriptor.value) {
      store->appendChild(descriptor.value);
    }
  }
  entity->getSymbolProperties()[key->getStore()] = descriptor;
  store->appendChild(key->getStore());
  return ctx->truly();
}
common::AutoPtr<JSValue> JSValue::setPropertyDescriptor(
    common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> name,
    const common::AutoPtr<JSValue> &value, bool configurable, bool enumable,
    bool writable) {
  return setPropertyDescriptor(ctx, name,
                               JSObjectEntity::JSField{
                                   .configurable = configurable,
                                   .enumable = enumable,
                                   .value = (JSStore *)value->getStore(),
                                   .writable = writable,
                                   .get = nullptr,
                                   .set = nullptr,
                               });
}

common::AutoPtr<JSValue> JSValue::setPropertyDescriptor(
    common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> name,
    const common::AutoPtr<JSValue> &get, const common::AutoPtr<JSValue> &set,
    bool configurable, bool enumable) {
  return setPropertyDescriptor(ctx, name,
                               JSObjectEntity::JSField{
                                   .configurable = configurable,
                                   .enumable = enumable,
                                   .value = nullptr,
                                   .writable = false,
                                   .get = (JSStore *)get->getStore(),
                                   .set = (JSStore *)set->getStore(),
                               });
}
common::AutoPtr<JSValue> JSValue::getProperty(common::AutoPtr<JSContext> ctx,
                                              common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (key->getType() != JSValueType::JS_SYMBOL) {
    if (getType() == JSValueType::JS_ARRAY) {
      auto num = key->toNumber(ctx)->getNumber();
      if (num.has_value()) {
        return getIndex(ctx, num.value());
      }
    }
    return getProperty(ctx, key->toString(ctx)->getString().value());
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
    auto num = key->toNumber(ctx)->getNumber();
    if (num.has_value()) {
      return setIndex(ctx, num.value(), field);
    }
  }
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return setProperty(ctx, key->toString(ctx)->getString().value(), field);
  }
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto store = self->getStore();
  auto descriptor = self->getOwnPropertyDescriptor(ctx, name);
  if (descriptor && descriptor->value == field->getStore()) {
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
      if (descriptor->value != field->getStore()) {
        store->removeChild(descriptor->value);
        ctx->getScope()->getRoot()->appendChild(descriptor->value);
        store->appendChild((JSStore *)field->getStore());
        descriptor->value = (JSStore *)(field->getStore());
      }
    } else {
      auto setter = ctx->createValue(descriptor->set);
      return setter->apply(ctx, self, {field});
    }
  } else {
    entity->getSymbolProperties()[key->getStore()] = JSObjectEntity::JSField{
        .configurable = true,
        .enumable = true,
        .value = (JSStore *)field->getStore(),
        .writable = true,
        .get = nullptr,
        .set = nullptr,
    };
    store->appendChild((JSStore *)field->getStore());
    store->appendChild((JSStore *)key->getStore());
  }
  return ctx->truly();
}

common::AutoPtr<JSValue>
JSValue::removeProperty(common::AutoPtr<JSContext> ctx,
                        common::AutoPtr<JSValue> name) {
  auto key = name->toPrimitive(ctx);
  if (getType() == JSValueType::JS_ARRAY) {
    auto num = key->toNumber(ctx)->getNumber();
    if (num.has_value()) {
      auto &items = getEntity<JSArrayEntity>()->getItems();
      if (num.value() < items.size()) {
        if (items[num.value()] != nullptr) {
          _store->removeChild(items[num.value()]);
        }
        items[num.value()] = nullptr;
      }
      return ctx->truly();
    }
  }
  if (key->getType() != JSValueType::JS_SYMBOL) {
    return removeProperty(ctx, key->toString(ctx)->getString().value());
  }
  auto self = pack(ctx);
  auto entity = self->getEntity<JSObjectEntity>();
  auto store = self->getStore();
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
    store->removeChild(descriptor->value);
    ctx->getScope()->getRoot()->appendChild(descriptor->value);
  }
  if (descriptor->get) {
    store->removeChild(descriptor->get);
    ctx->getScope()->getRoot()->appendChild(descriptor->get);
  }
  if (descriptor->set) {
    store->removeChild(descriptor->set);
    ctx->getScope()->getRoot()->appendChild(descriptor->set);
  }
  entity->getSymbolProperties().erase(key->getStore());
  store->removeChild(key->getStore());
  ctx->getScope()->getRoot()->appendChild(key->getStore());
  return ctx->truly();
}

common::AutoPtr<JSValue> JSValue::unaryPlus(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(getEntity<JSInfinityEntity>()->isNegative());
  }
  auto value = toNumber(ctx)->getNumber();
  if (value.has_value()) {
    return ctx->createNumber(value.value());
  }
  return ctx->NaN();
}

common::AutoPtr<JSValue>
JSValue::unaryNetation(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
    return ctx->createInfinity(!getEntity<JSInfinityEntity>()->isNegative());
  }
  if (getType() == JSValueType::JS_BIGINT) {
    return ctx->createBigInt(-getEntity<JSBigIntEntity>()->getValue());
  }
  auto value = toNumber(ctx)->getNumber();
  if (value.has_value()) {
    return ctx->createNumber(-value.value());
  }
  return ctx->NaN();
}

void JSValue::increment(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
  }
  if (getType() == JSValueType::JS_BIGINT) {
    auto &value = getEntity<JSBigIntEntity>()->getValue();
    value += 1;
  }
  auto value = toNumber(ctx)->getNumber();
  if (value.has_value()) {
    setNumber(value.value() + 1);
  } else {
    setEntity(new JSNaNEntity());
  }
}

void JSValue::decrement(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_INFINITY) {
  }
  if (getType() == JSValueType::JS_BIGINT) {
    auto &value = getEntity<JSBigIntEntity>()->getValue();
    value -= 1;
  }
  auto value = toNumber(ctx)->getNumber();
  if (value.has_value()) {
    setNumber(value.value() - 1);
  } else {
    setEntity(new JSNaNEntity());
  }
}

common::AutoPtr<JSValue> JSValue::logicalNot(common::AutoPtr<JSContext> ctx) {
  return ctx->createBoolean(!toBoolean(ctx)->getBoolean().value());
}

common::AutoPtr<JSValue> JSValue::bitwiseNot(common::AutoPtr<JSContext> ctx) {
  if (getType() == JSValueType::JS_BIGINT) {
    auto value = getEntity<JSBigIntEntity>()->getValue();
    return ctx->createBigInt(~value);
  }
  auto num = toNumber(ctx)->getNumber();
  if (num.has_value()) {
    return ctx->createNumber(~(int64_t)num.value());
  }
  return ctx->createNumber(~0);
}

common::AutoPtr<JSValue> JSValue::add(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
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
  if (left->getTypeName() == L"number" && right->getTypeName() == L"number") {
    if (left->isNaN() || right->isNaN()) {
      return ctx->NaN();
    }
    if (left->isInfinity() && right->isInfinity()) {
      if (left->getEntity<JSInfinityEntity>()->isNegative() !=
          right->getEntity<JSInfinityEntity>()->isNegative()) {
        return ctx->NaN();
      }
    }
    if (left->isInfinity()) {
      return ctx->createValue(left);
    }
    if (right->isInfinity()) {
      return ctx->createValue(right);
    }
    return ctx->createNumber(left->getNumber().value() +
                             right->getNumber().value());
  }
  return ctx->createString(left->toString(ctx)->getString().value() +
                           right->toString(ctx)->getString().value());
}

common::AutoPtr<JSValue> JSValue::sub(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
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
  left = left->toNumber(ctx);
  right = right->toNumber(ctx);
  if (left->isNaN() || right->isNaN()) {
    return ctx->NaN();
  }
  if (left->isInfinity() && right->isInfinity()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative() !=
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
  return ctx->createNumber(left->getNumber().value() -
                           right->getNumber().value());
}

common::AutoPtr<JSValue> JSValue::mul(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
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
  left = left->toNumber(ctx);
  right = right->toNumber(ctx);
  if (left->isNaN() || right->isNaN()) {
    return ctx->NaN();
  }
  if (left->isInfinity() && right->isInfinity()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative() !=
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
  return ctx->createNumber(left->getNumber().value() *
                           right->getNumber().value());
}

common::AutoPtr<JSValue> JSValue::div(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
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
  left = left->toNumber(ctx);
  right = right->toNumber(ctx);
  if (left->isNaN() || right->isNaN()) {
    return ctx->NaN();
  }
  if (left->isInfinity() && right->isInfinity()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative() !=
        right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->NaN();
    }
  }
  if (left->isInfinity()) {
    return ctx->createValue(left);
  }
  if (right->isInfinity()) {
    return ctx->createNumber(0);
  }
  if (right->getNumber().value() == 0) {
    return ctx->createInfinity(left->getNumber().value() < 0);
  }
  return ctx->createNumber(left->getNumber().value() /
                           right->getNumber().value());
}

common::AutoPtr<JSValue> JSValue::mod(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
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
  left = left->toNumber(ctx);
  right = right->toNumber(ctx);
  if (left->isNaN() || right->isNaN()) {
    return ctx->NaN();
  }
  if (left->isInfinity() && right->isInfinity()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative() !=
        right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->NaN();
    }
  }
  if (left->isInfinity()) {
    return ctx->createValue(left);
  }
  if (right->isInfinity()) {
    return ctx->createNumber(0);
  }
  if (right->getNumber().value() == 0) {
    return ctx->NaN();
  }
  return ctx->createNumber((int64_t)left->getNumber().value() %
                           (int64_t)right->getNumber().value());
}

common::AutoPtr<JSValue> JSValue::pow(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(
          left->getEntity<JSBigIntEntity>()->getValue().pow(
              right->getEntity<JSBigIntEntity>()->getValue()));
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }

  left = left->toNumber(ctx);
  right = right->toNumber(ctx);
  if (left->isNaN() || right->isNaN()) {
    return ctx->NaN();
  }
  if (left->isInfinity()) {
    if (right->isInfinity()) {
      if (right->getEntity<JSInfinityEntity>()->isNegative()) {
        return ctx->createNumber();
      } else {
        return ctx->createInfinity();
      }
    }
    if (right->getNumber().value() == 0) {
      return ctx->createNumber(1);
    }
    if ((int64_t)right->getNumber().value() % 2 == 0) {
      return ctx->createInfinity();
    } else {
      return ctx->createValue(left);
    }
  }
  if (right->isInfinity()) {

    if (left->getNumber().value() == 1 || left->getNumber().value() == -1) {
      return ctx->NaN();
    }
    if (right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->createNumber();
    } else {
      return ctx->createInfinity();
    }
  }
  return ctx->createNumber(
      std::pow(left->getNumber().value(), right->getNumber().value()));
} // a**b

common::AutoPtr<JSValue> JSValue::shl(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(
          left->getEntity<JSBigIntEntity>()->getValue()
          << right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (leftval.has_value() && rightval.has_value()) {
    int32_t val = (int32_t)leftval.value();
    int64_t arg = (int64_t)rightval.value();
    while (arg > 256) {
      arg -= 256;
    }
    while (arg < 0) {
      arg += 256;
    }
    return ctx->createNumber(val << (uint8_t)arg);
  }
  return ctx->createNumber();
} // a<<b

common::AutoPtr<JSValue> JSValue::shr(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() >>
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (leftval.has_value() && rightval.has_value()) {
    int32_t val = (int32_t)leftval.value();
    int64_t arg = (int64_t)rightval.value();
    while (arg > 256) {
      arg -= 256;
    }
    while (arg < 0) {
      arg += 256;
    }
    return ctx->createNumber(val >> (uint8_t)arg);
  }
  return ctx->createNumber();
} // a>>b

common::AutoPtr<JSValue> JSValue::ushr(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() >>
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"BigInts have no unsigned right shift, use >> instead");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (leftval.has_value() && rightval.has_value()) {
    uint32_t val = (uint32_t)leftval.value();
    int64_t arg = (int64_t)rightval.value();
    while (arg > 256) {
      arg -= 256;
    }
    while (arg < 0) {
      arg += 256;
    }
    return ctx->createNumber(val >> (uint8_t)arg);
  }
  return ctx->createNumber();
} // a>>>b

common::AutoPtr<JSValue> JSValue::ge(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBoolean(left->getEntity<JSBigIntEntity>()->getValue() >=
                                right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  if (left->isNaN() || right->isNaN()) {
    return ctx->falsely();
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (left->isInfinity() && rightval.has_value()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->falsely();
    } else {
      return ctx->truly();
    }
  }
  if (right->isInfinity() && leftval.has_value()) {
    if (right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->truly();
    } else {
      return ctx->falsely();
    }
  }
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createBoolean(leftval.value() >= rightval.value());
  }
  if (leftval.has_value() || rightval.has_value()) {
    return ctx->falsely();
  }
  auto ls = left->toString(ctx)->getString().value();
  auto rs = right->toString(ctx)->getString().value();
  return ctx->createBoolean(ls >= rs);
} // a>=b

common::AutoPtr<JSValue> JSValue::le(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBoolean(left->getEntity<JSBigIntEntity>()->getValue() <=
                                right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (left->isNaN() || right->isNaN()) {
    return ctx->falsely();
  }
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createBoolean(leftval.value() <= rightval.value());
  }
  if (left->isInfinity() && rightval.has_value()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->truly();
    } else {
      return ctx->falsely();
    }
  }
  if (right->isInfinity() && leftval.has_value()) {
    if (right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->falsely();
    } else {
      return ctx->truly();
    }
  }

  if (leftval.has_value() || rightval.has_value()) {
    return ctx->falsely();
  }
  auto ls = left->toString(ctx)->getString().value();
  auto rs = right->toString(ctx)->getString().value();
  return ctx->createBoolean(ls <= rs);
} // a<=b

common::AutoPtr<JSValue> JSValue::gt(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBoolean(left->getEntity<JSBigIntEntity>()->getValue() >
                                right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (left->isNaN() || right->isNaN()) {
    return ctx->falsely();
  }
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createBoolean(leftval.value() > rightval.value());
  }
  if (left->isInfinity() && rightval.has_value()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->falsely();
    } else {
      return ctx->truly();
    }
  }
  if (right->isInfinity() && leftval.has_value()) {
    if (right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->truly();
    } else {
      return ctx->falsely();
    }
  }

  if (leftval.has_value() || rightval.has_value()) {
    return ctx->falsely();
  }
  auto ls = left->toString(ctx)->getString().value();
  auto rs = right->toString(ctx)->getString().value();
  return ctx->createBoolean(ls > rs);
} // a>b

common::AutoPtr<JSValue> JSValue::lt(common::AutoPtr<JSContext> ctx,
                                     common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBoolean(left->getEntity<JSBigIntEntity>()->getValue() <
                                right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (left->isNaN() || right->isNaN()) {
    return ctx->falsely();
  }
  if (leftval.has_value() && rightval.has_value()) {
    return ctx->createBoolean(leftval.value() < rightval.value());
  }
  if (left->isInfinity() && rightval.has_value()) {
    if (left->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->truly();
    } else {
      return ctx->falsely();
    }
  }
  if (right->isInfinity() && leftval.has_value()) {
    if (right->getEntity<JSInfinityEntity>()->isNegative()) {
      return ctx->falsely();
    } else {
      return ctx->truly();
    }
  }

  if (leftval.has_value() || rightval.has_value()) {
    return ctx->falsely();
  }
  auto ls = left->toString(ctx)->getString().value();
  auto rs = right->toString(ctx)->getString().value();
  return ctx->createBoolean(ls < rs);
} // a<b

common::AutoPtr<JSValue> JSValue::and_(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() &
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (leftval.has_value() && rightval.has_value()) {
    int32_t val = (int32_t)leftval.value();
    int32_t arg = (int64_t)rightval.value();
    return ctx->createNumber(val & arg);
  }
  return ctx->createNumber();
} // a&b

common::AutoPtr<JSValue> JSValue::or_(common::AutoPtr<JSContext> ctx,
                                      common::AutoPtr<JSValue> another) {
  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() |
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (leftval.has_value() && rightval.has_value()) {
    int32_t val = (int32_t)leftval.value();
    int32_t arg = (int64_t)rightval.value();
    return ctx->createNumber(val | arg);
  }
  if (leftval.has_value()) {
    return ctx->createNumber(leftval.value());
  }
  if (rightval.has_value()) {
    return ctx->createNumber(rightval.value());
  }
  return ctx->createNumber();
} // a|b

common::AutoPtr<JSValue> JSValue::xor_(common::AutoPtr<JSContext> ctx,
                                       common::AutoPtr<JSValue> another) {

  auto left = toPrimitive(ctx);
  auto right = another->toPrimitive(ctx);
  if (left->getType() == JSValueType::JS_BIGINT ||
      right->getType() == JSValueType::JS_BIGINT) {
    if (left->getType() == JSValueType::JS_BIGINT &&
        right->getType() == JSValueType::JS_BIGINT) {
      if (right->getEntity<JSBigIntEntity>()->getValue() == 0) {
        throw error::JSRangeError(L"Division by zero");
      }
      return ctx->createBigInt(left->getEntity<JSBigIntEntity>()->getValue() ^
                               right->getEntity<JSBigIntEntity>()->getValue());
    } else {
      throw error::JSTypeError(
          L"Cannot mix BigInt and other types, use explicit conversions");
    }
  }
  auto leftval = left->toNumber(ctx)->getNumber();
  auto rightval = right->toNumber(ctx)->getNumber();
  if (leftval.has_value() && rightval.has_value()) {
    int32_t val = (int32_t)leftval.value();
    int32_t arg = (int64_t)rightval.value();
    return ctx->createNumber(val ^ arg);
  }
  if (leftval.has_value()) {
    return ctx->createNumber(leftval.value());
  }
  if (rightval.has_value()) {
    return ctx->createNumber(rightval.value());
  }
  return ctx->createNumber();
} // a^b

common::AutoPtr<JSValue> JSValue:: instanceof
    (common::AutoPtr<JSContext> ctx, common::AutoPtr<JSValue> another) {
  if (another->getType() < JSValueType::JS_OBJECT) {
    throw error::JSTypeError(
        L"Right-hand side of 'instanceof' is not an object");
  }
  if (getType() < JSValueType::JS_OBJECT) {
    return ctx->falsely();
  }
  auto prototype = another->getProperty(ctx, L"prototype");
  auto current = pack(ctx)->getPrototype(ctx);
  while (current->getType() >= JSValueType::JS_OBJECT) {
    if (current->getStore() == prototype->getStore()) {
      return ctx->truly();
    }
    current = current->getPrototype(ctx);
  }
  return ctx->falsely();
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
    return ctx->createNumber(toNumber(ctx)->getNumber().value())
        ->equal(ctx, another);
  }
  if (another->getType() == JSValueType::JS_BOOLEAN) {
    return equal(
        ctx, ctx->createNumber(another->toNumber(ctx)->getNumber().value()));
  }
  if (getType() == JSValueType::JS_NUMBER &&
      another->getType() == JSValueType::JS_STRING) {
    auto num = another->toNumber(ctx)->getNumber();
    if (num.has_value()) {
      return ctx->createBoolean(getNumber().value() == num.value());
    }
  }
  if (getType() == JSValueType::JS_STRING &&
      another->getType() == JSValueType::JS_NUMBER) {
    auto num = toNumber(ctx)->getNumber();
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
JSValue::strictNotEqual(common::AutoPtr<JSContext> ctx,
                        common::AutoPtr<JSValue> another) {
  return ctx->createBoolean(!strictEqual(ctx, another)->getBoolean().value());
}