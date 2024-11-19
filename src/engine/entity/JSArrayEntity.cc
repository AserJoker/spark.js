#include "engine/entity/JSArrayEntity.hpp"
#include "engine/base/JSValueType.hpp"
using namespace spark;
using namespace spark::engine;
JSArrayEntity::JSArrayEntity() { _type = JSValueType::JS_ARRAY; };

const std::vector<JSEntity *> &JSArrayEntity::getItems() const {
  return _items;
}

std::vector<JSEntity *> &JSArrayEntity::getItems() { return _items; }