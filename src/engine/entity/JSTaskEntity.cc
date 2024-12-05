#include "engine/entity/JSTaskEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
using namespace spark;
using namespace spark::engine;
JSTaskEntity::JSTaskEntity(JSEntity *value, size_t address)
    : JSEntity(JSValueType::JS_TASK), _value(value), _address(address) {
  appendChild(value);
}

JSEntity *JSTaskEntity::getValue() { return _value; }

size_t JSTaskEntity::getAddress() { return _address; }