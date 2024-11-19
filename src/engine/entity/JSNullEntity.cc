#include "engine/entity/JSNullEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSNullEntity::JSNullEntity() : JSEntity(JSValueType::JS_NULL) {}

std::wstring JSNullEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"null";
};

std::optional<double>
JSNullEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return 0;
};
