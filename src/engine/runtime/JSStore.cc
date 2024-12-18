#include "engine/runtime/JSStore.hpp"
#include "engine/entity/JSEntity.hpp"
#include <algorithm>
using namespace spark;
using namespace spark::engine;
JSStore::JSStore(const common::AutoPtr<JSEntity> &entity) : _entity(entity) {}
JSStore::~JSStore() {
  for (auto &child : _children) {
    auto it = std::find(child->_parents.begin(), child->_parents.end(), this);
    if (it != child->_parents.end()) {
      child->_parents.erase(it);
    }
  }
  _children.clear();
  for (auto &parent : _parents) {
    auto it =
        std::find(parent->_children.begin(), parent->_children.end(), this);
    if (it != parent->_children.end()) {
      parent->_children.erase(it);
    }
  }
  _parents.clear();
}
void JSStore::appendChild(JSStore *store) {
  store->_parents.push_back(this);
  _children.push_back(store);
}

void JSStore::removeChild(JSStore *store) {
  auto it = std::find(store->_parents.begin(), store->_parents.end(), this);
  if (it != store->_parents.end()) {
    store->_parents.erase(it);
  }
  it = std::find(_children.begin(), _children.end(), store);
  if (it != _children.end()) {
    _children.erase(it);
  }
}

common::AutoPtr<JSEntity> JSStore::getEntity() { return _entity; }

const common::AutoPtr<JSEntity> &JSStore::getEntity() const { return _entity; }

void JSStore::setEntity(common::AutoPtr<JSEntity> entity) { _entity = entity; }

const std::vector<JSStore *> &JSStore::getParent() const { return _parents; }

std::vector<JSStore *> &JSStore::getParent() { return _parents; }

const std::vector<JSStore *> &JSStore::getChildren() const { return _children; }

std::vector<JSStore *> &JSStore::getChildren() { return _children; }