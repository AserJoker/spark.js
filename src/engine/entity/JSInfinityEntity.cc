#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSInfinityEntity::JSInfinityEntity() : JSBaseEntity(JSValueType::JS_INFINITY) {}

std::wstring JSInfinityEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"Infinity";
};

bool JSInfinityEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
};

std::wstring
JSInfinityEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"number";
}