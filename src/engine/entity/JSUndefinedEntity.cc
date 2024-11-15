#include "engine/entity/JSUndefinedEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSUndefinedEntity::JSUndefinedEntity()
    : JSBaseEntity(JSValueType::JS_UNDEFINED) {}

std::wstring JSUndefinedEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"undefined";
};

std::wstring
JSUndefinedEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"undefined";
};