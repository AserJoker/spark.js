#include "engine/entity/JSArrayEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
using namespace spark;
using namespace spark::engine;
JSArrayEntity::JSArrayEntity(JSStore *prototype) : JSObjectEntity(prototype) {
  _type = JSValueType::JS_ARRAY;
};

const std::vector<JSStore *> &JSArrayEntity::getItems() const {
  return _items;
}

std::vector<JSStore *> &JSArrayEntity::getItems() { return _items; }