#include "engine/entity/JSEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::engine;
JSEntity::JSEntity(const JSValueType &type) : _type(type) {}

JSEntity::~JSEntity() {}

void JSEntity::appendChild(JSEntity *entity) {
  entity->_parents.push_back(this);
  _children.push_back(entity);
}

void JSEntity::removeChild(JSEntity *entity) {
  auto it = std::find(entity->_parents.begin(), entity->_parents.end(), this);
  if (it != entity->_parents.end()) {
    entity->_parents.erase(it);
  }
  it = std::find(_children.begin(), _children.end(), entity);
  if (it != _children.end()) {
    _children.erase(it);
  }
}

std::vector<JSEntity *> &JSEntity::getParent() { return _parents; }

std::vector<JSEntity *> &JSEntity::getChildren() { return _children; }

const JSValueType &JSEntity::getType() const { return _type; }

std::wstring JSEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"[internal Internal]";
}

std::optional<double> JSEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return std::nullopt;
}

bool JSEntity::toBoolean(common::AutoPtr<JSContext> ctx) const { return false; }
