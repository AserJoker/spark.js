#include "engine/entity/JSEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSEntity::JSEntity(const JSValueType &type) : _type(type) {}

JSEntity::~JSEntity() {}

void JSEntity::appendChild(JSEntity *entity) {
  entity->_parents.pushBack(this);
  _children.pushBack(entity);
}

void JSEntity::removeChild(JSEntity *entity) {
  entity->_parents.erase(this);
  _children.erase(entity);
}

common::Array<JSEntity *> &JSEntity::getParent() { return _parents; }

common::Array<JSEntity *> &JSEntity::getChildren() { return _children; }

const JSValueType &JSEntity::getType() const { return _type; }

std::wstring JSEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"[internal Internal]";
}

std::optional<double> JSEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return std::nullopt;
}

bool JSEntity::toBoolean(common::AutoPtr<JSContext> ctx) const { return false; }
