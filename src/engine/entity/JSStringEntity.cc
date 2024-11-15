#include "engine/entity/JSStringEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include <string>
using namespace spark;
using namespace spark::engine;
JSStringEntity::JSStringEntity(const std::wstring &value)
    : JSBaseEntity(JSValueType::JS_STRING, value) {}

std::wstring &JSStringEntity::getValue() { return getData(); }

const std::wstring &JSStringEntity::getValue() const { return getData(); }

std::wstring JSStringEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return getValue();
};

bool JSStringEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return getValue().length() != 0;
};

std::wstring JSStringEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"string";
};