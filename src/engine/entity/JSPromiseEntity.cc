#include "engine/entity/JSPromiseEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
using namespace spark;
using namespace spark::engine;
JSPromiseEntity::JSPromiseEntity(JSStore *prototype)
    : JSObjectEntity(prototype), _status(Status::PENDING), _value(nullptr) {}

JSStore *JSPromiseEntity::getValue() { return _value; }

void JSPromiseEntity::setValue(JSStore *value) { _value = value; }

JSPromiseEntity::Status &JSPromiseEntity::getStatus() { return _status; }

std::vector<JSStore *> &JSPromiseEntity::getFulfilledCallbacks() {
  return _fulfilledCallbacks;
}

std::vector<JSStore *> &JSPromiseEntity::getRejectedCallbacks() {
  return _rejectedCallbacks;
}

std::vector<JSStore *> &JSPromiseEntity::getFinallyCallbacks() {
  return _finallyCallbacks;
}