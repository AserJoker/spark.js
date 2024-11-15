#include "engine/entity/JSBooleanEntity.hpp"
using namespace spark;
using namespace spark::engine;

bool &JSBooleanEntity::getValue() { return getData().value; }

bool JSBooleanEntity::getValue() const { return getData().value; }

JSBooleanEntity::JSBooleanEntity(bool value)
    : JSBaseEntity(JSValueType::JS_BOOLEAN, {value}) {}

std::wstring JSBooleanEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return getValue() ? L"true" : L"false";
};

std::optional<double>
JSBooleanEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return getValue() ? 1 : 0;
};

bool JSBooleanEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return getValue();
};

std::wstring
JSBooleanEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"boolean";
};