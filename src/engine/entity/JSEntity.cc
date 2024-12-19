#include "engine/entity/JSEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::engine;
JSEntity::JSEntity(const JSValueType &type) : _type(type), _opaque(nullptr) {}

JSEntity::~JSEntity() { _opaque = nullptr; }

const JSValueType &JSEntity::getType() const { return _type; }

std::wstring JSEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"[internal Internal]";
}

std::optional<double> JSEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return std::nullopt;
}

bool JSEntity::toBoolean(common::AutoPtr<JSContext> ctx) const { return false; }
