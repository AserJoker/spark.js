#include "engine/entity/JSSymbolEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
#include "error/JSTypeError.hpp"


using namespace spark;
using namespace spark::engine;

JSSymbolEntity::JSSymbolEntity(const std::wstring &description)
    : JSBaseEntity(JSValueType::JS_SYMBOL, description) {}

const std::wstring &JSSymbolEntity::getDescription() const { return getData(); }

std::wstring JSSymbolEntity::toString(common::AutoPtr<JSContext> ctx) const {
  throw error::JSTypeError(L"Cannot convert a Symbol value to a string");
};

std::optional<double>
JSSymbolEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  throw error::JSTypeError(L"Cannot convert a Symbol value to a number");
};

bool JSSymbolEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
};

std::wstring JSSymbolEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"symbol";
};