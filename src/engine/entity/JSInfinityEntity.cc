#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
using namespace spark;
using namespace spark::engine;
JSInfinityEntity::JSInfinityEntity(bool negative)
    : JSEntity(JSValueType::JS_INFINITY), _negative(negative) {}

std::wstring JSInfinityEntity::toString(common::AutoPtr<JSContext> ctx) const {
  if (_negative) {
    return L"-Infinity";
  }
  return L"Infinity";
};

bool JSInfinityEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
};

std::wstring
JSInfinityEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"number";
}
bool JSInfinityEntity::isNegative() const { return _negative; }

void JSInfinityEntity::negative() { _negative = !_negative; }