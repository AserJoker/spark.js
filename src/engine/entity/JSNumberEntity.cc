#include "engine/entity/JSNumberEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;

JSNumberEntity::JSNumberEntity(double value)
    : JSEntity(JSValueType::JS_NUMBER), _value(value){};

double &JSNumberEntity::getValue() {
  return _value;
}

const double JSNumberEntity::getValue() const { return _value; }

std::wstring JSNumberEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return fmt::format(L"{:g}", getValue());
};

std::optional<double>
JSNumberEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return getValue();
};

bool JSNumberEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return getValue() != 0;
};