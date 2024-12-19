#include "engine/runtime/JSScope.hpp"
#include "common/Map.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSValue.hpp"
#include <fmt/xchar.h>
#include <string>

using namespace spark;
using namespace spark::engine;
JSScope::JSScope(const common::AutoPtr<JSScope> &parent) {
  _parent = (JSScope *)parent.getRawPointer();
  _root = new JSStore(new JSEntity(JSValueType::JS_INTERNAL));
  if (_parent) {
    _parent->_root->appendChild(_root);
    _parent->_children.push_back(this);
  }
}

JSScope::~JSScope() {
  for (auto &child : _children) {
    child->_parent = nullptr;
  }
  _children.clear();
  _values.clear();
  _anonymousValues.clear();
  std::vector<JSStore *> workflow;
  while (_root->getChildren().size()) {
    auto store = *_root->getChildren().begin();
    workflow.push_back(store);
    _root->removeChild(store);
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
  common::Map<JSStore *, bool> cache;
  std::vector<JSStore *> destroyed;
  while (!workflow.empty()) {
    auto store = *workflow.begin();
    workflow.erase(workflow.begin());
    if (!cache.contains(store)) {
      bool isAlived = isEntityAlived(store, cache);
      if (!isAlived) {
        destroyed.push_back(store);
      }
      cache[store] = isAlived;
      for (auto &child : store->getChildren()) {
        if (!cache.contains(child)) {
          auto it = std::find(workflow.begin(), workflow.end(), child);
          if (it == workflow.end()) {
            workflow.push_back(child);
          }
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

bool JSScope::isEntityAlived(JSStore *store,
                             common::Map<JSStore *, bool> &cache) {
  std::vector<JSStore *> workflow = {store};
  std::vector<JSStore *> alivedCache;
  while (!workflow.empty()) {
    auto store = *workflow.rbegin();
    workflow.pop_back();
    if (cache.contains(store) && cache.at(store)) {
      return true;
    }
    if (store->getEntity()->getType() == JSValueType::JS_INTERNAL) {
      return true;
    }
    alivedCache.push_back(store);
    for (auto &parent : store->getParent()) {
      auto it = std::find(alivedCache.begin(), alivedCache.end(), parent);
      if (it == alivedCache.end()) {
        workflow.push_back(parent);
      }
    }
  }
  return false;
}

JSStore *JSScope::getRoot() { return _root; }

common::AutoPtr<JSScope> JSScope::getRootScope() {
  auto root = this;
  while (root->_parent) {
    root = root->_parent;
  }
  return root;
}
common::AutoPtr<JSScope> JSScope::getParent() { return _parent; }

void JSScope::removeChild(const common::AutoPtr<JSScope> &child) {
  auto it = std::find(_children.begin(), _children.end(), child);
  if (it != _children.end()) {
    _children.erase(it);
  }
}

common::AutoPtr<JSValue> JSScope::createValue(JSStore *store,
                                              const std::wstring &name) {
  _root->appendChild(store);
  auto value = new JSValue(this, store);
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