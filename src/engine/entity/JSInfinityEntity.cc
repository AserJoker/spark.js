#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSInfinityEntity::JSInfinityEntity(bool negative)
    : JSBaseEntity(JSValueType::JS_INFINITY, {negative}) {}

std::wstring JSInfinityEntity::toString(common::AutoPtr<JSContext> ctx) const {
  if (getData().negative) {
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
bool JSInfinityEntity::isNegative() const { return getData().negative; }

void JSInfinityEntity::negative() { getData().negative = !getData().negative; }