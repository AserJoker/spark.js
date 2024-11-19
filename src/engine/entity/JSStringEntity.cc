#include "engine/entity/JSStringEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/base/JSValueType.hpp"
#include <string>
using namespace spark;
using namespace spark::engine;
JSStringEntity::JSStringEntity(const std::wstring &value)
    : JSEntity(JSValueType::JS_STRING), _value(value) {}

std::wstring &JSStringEntity::getValue() { return _value; }

const std::wstring &JSStringEntity::getValue() const { return _value; }

std::wstring JSStringEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return getValue();
};

bool JSStringEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return getValue().length() != 0;
};
