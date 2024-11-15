#include "engine/entity/JSNaNEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSNaNEntity::JSNaNEntity() : JSBaseEntity(JSValueType::JS_NAN) {}

std::wstring JSNaNEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"NaN";
};

std::wstring JSNaNEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"number";
};