#include "engine/runtime/JSScope.hpp"
#include "common/Array.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include <fmt/xchar.h>
#include <string>

using namespace spark;
using namespace spark::engine;
JSScope::JSScope(JSScope *parent) {
  _parent = parent;
  _root = new JSEntity();
  if (_parent) {
    _parent->_root->appendChild(_root);
    _parent->_children.pushBack(this);
  }
}

JSScope::~JSScope() {
  for (auto &child : _children) {
    delete child;
  }
  _children.clear();
  for (auto &[_, value] : _values) {
    value->setEntity(nullptr);
  }
  _values.clear();
  for (auto &value : _anonymousValues) {
    value->setEntity(nullptr);
  }
  _anonymousValues.clear();
  common::Array<JSEntity *> workflow;
  while (_root->getChildren().size()) {
    auto entity = *_root->getChildren().begin();
    workflow.pushBack(entity);
    _root->removeChild(entity);
  }
  if (_parent) {
    _parent->_children.erase(this);
    _parent->_root->removeChild(_root);
  }
  common::Map<JSEntity *, bool> cache;
  common::Array<JSEntity *> destroyed;
  while (!workflow.empty()) {
    auto entity = *workflow.rbegin();
    workflow.popBack();
    if (!cache.contains(entity)) {
      bool isAlived = isEntityAlived(entity, cache);
      if (!isAlived) {
        destroyed.pushBack(entity);
      }
      cache[entity] = isAlived;
      for (auto &child : entity->getChildren()) {
        if (!cache.contains(child)) {
          workflow.pushBack(child);
        }
      }
    }
  }
  while (!destroyed.empty()) {
    delete *destroyed.rbegin();
    destroyed.popBack();
  }
  delete _root;
}

bool JSScope::isEntityAlived(JSEntity *entity,
                             common::Map<JSEntity *, bool> &cache) {
  common::Array<JSEntity *> workflow = {entity};
  common::Array<JSEntity *> alivedCache;
  while (!workflow.empty()) {
    auto entity = *workflow.rbegin();
    workflow.popBack();
    if (cache.contains(entity) && cache.at(entity)) {
      return true;
    }
    if (entity->getType() == JSValueType::JS_INTERNAL) {
      return true;
    }
    alivedCache.pushBack(entity);
    for (auto &parent : entity->getParent()) {
      if (!alivedCache.contains(parent)) {
        workflow.pushBack(parent);
      }
    }
  }
  return false;
}

JSEntity *JSScope::getRoot() { return _root; }

JSScope *JSScope::getRootScope() {
  auto root = this;
  while (root->_parent) {
    root = root->_parent;
  }
  return root;
}
JSScope *JSScope::getParent() { return _parent; }

common::AutoPtr<JSValue> JSScope::createValue(JSEntity *entity,
                                              const std::wstring &name) {
  _root->appendChild(entity);
  auto value = new JSValue(this, entity);
  if (!name.empty()) {
    _values[name] = value;
  } else {
    _anonymousValues.pushBack(value);
  }
  return value;
}

const common::Map<std::wstring, common::AutoPtr<JSValue>> &
JSScope::getValues() const {
  return _values;
}

common::AutoPtr<JSValue> JSScope::getValue(const std::wstring &name) const {
  if (!_values.contains(name)) {
    if (_parent) {
      return _parent->getValue(name);
    }
    return nullptr;
  }
  return _values.at(name);
}