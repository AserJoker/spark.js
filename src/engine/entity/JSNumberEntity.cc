#include "engine/entity/JSNumberEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;

JSNumberEntity::JSNumberEntity(double value)
    : JSBaseEntity(JSValueType::JS_NUMBER, value){};

double &JSNumberEntity::getValue() { return getData(); }

const double JSNumberEntity::getValue() const { return getData(); }

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

std::wstring JSNumberEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"number";
};