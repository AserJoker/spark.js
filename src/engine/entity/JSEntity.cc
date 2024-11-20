#include "engine/entity/JSEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/runtime/JSContext.hpp"
using namespace spark;
using namespace spark::engine;
JSEntity::JSEntity(const JSValueType &type) : _type(type), _opaque(nullptr) {}

JSEntity::~JSEntity() {
  for (auto &child : _children) {
    auto it = std::find(child->_parents.begin(), child->_parents.end(), this);
    if (it != child->_parents.end()) {
      child->_parents.erase(it);
    }
  }
  for (auto &parent : _parents) {
    auto it =
        std::find(parent->_children.begin(), parent->_children.end(), this);
    if (it != parent->_children.end()) {
      parent->_children.erase(it);
    }
  }
}

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

std::any &JSEntity::getOpaque() { return _opaque; }

const std::any &JSEntity::getOpaque() const { return _opaque; }

void JSEntity::setOpaque(const std::any &value) { _opaque = value; }

const JSValueType &JSEntity::getType() const { return _type; }

std::wstring JSEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return L"[internal Internal]";
}

std::optional<double> JSEntity::toNumber(common::AutoPtr<JSContext> ctx) const {
  return std::nullopt;
}

bool JSEntity::toBoolean(common::AutoPtr<JSContext> ctx) const { return false; }
