#include "engine/runtime/JSScope.hpp"
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
    _parent->_children.push_back(this);
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
  std::vector<JSEntity *> workflow;
  while (_root->getChildren().size()) {
    auto entity = *_root->getChildren().begin();
    workflow.push_back(entity);
    _root->removeChild(entity);
  }
  if (_parent) {
    auto it =
        std::find(_parent->_children.begin(), _parent->_children.end(), this);
    if (it != _parent->_children.end()) {
      _parent->_children.erase(it);
    }
    _parent->_root->removeChild(_root);
    _parent = nullptr;
  }
  common::Map<JSEntity *, bool> cache;
  std::vector<JSEntity *> destroyed;
  while (!workflow.empty()) {
    auto entity = *workflow.rbegin();
    workflow.pop_back();
    if (!cache.contains(entity)) {
      bool isAlived = isEntityAlived(entity, cache);
      if (!isAlived) {
        destroyed.push_back(entity);
      }
      cache[entity] = isAlived;
      for (auto &child : entity->getChildren()) {
        if (!cache.contains(child)) {
          workflow.push_back(child);
        }
      }
    }
  }
  while (!destroyed.empty()) {
    delete *destroyed.rbegin();
    destroyed.pop_back();
  }
  delete _root;
}

bool JSScope::isEntityAlived(JSEntity *entity,
                             common::Map<JSEntity *, bool> &cache) {
  std::vector<JSEntity *> workflow = {entity};
  std::vector<JSEntity *> alivedCache;
  while (!workflow.empty()) {
    auto entity = *workflow.rbegin();
    workflow.pop_back();
    if (cache.contains(entity) && cache.at(entity)) {
      return true;
    }
    if (entity->getType() == JSValueType::JS_INTERNAL) {
      return true;
    }
    alivedCache.push_back(entity);
    for (auto &parent : entity->getParent()) {
      auto it = std::find(alivedCache.begin(), alivedCache.end(), parent);
      if (it == alivedCache.end()) {
        workflow.push_back(parent);
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
    _anonymousValues.push_back(value);
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