#include "engine/entity/JSBigIntEntity.hpp"
#include "common/BigInt.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "error/JSTypeError.hpp"

using namespace spark;
using namespace spark::engine;

JSBigIntEntity::JSBigIntEntity(const common::BigInt<> &val)
    : JSEntity(JSValueType::JS_BIGINT), _value(common::BigInt<>(val)){};

const common::BigInt<> &JSBigIntEntity::getValue() const { return _value; }

common::BigInt<> &JSBigIntEntity::getValue() { return _value; }

std::wstring JSBigIntEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return getValue().toString();
}
std::optional<double>
JSBigIntEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  throw error::JSTypeError(L"Cannot convert a BigInt value to a number");
};

bool JSBigIntEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return getValue() != 0;
}

std::wstring JSBigIntEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"bigint";
}