#include "engine/entity/JSNaNEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
#include "engine/entity/JSEntity.hpp"
using namespace spark;
using namespace spark::engine;
JSNaNEntity::JSNaNEntity() : JSEntity(JSValueType::JS_NAN) {}

std::wstring JSNaNEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"NaN";
};
