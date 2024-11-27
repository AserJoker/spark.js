#include "engine/entity/JSArgumentEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
using namespace spark;
using namespace spark::engine;

JSArgumentEntity::JSArgumentEntity(JSEntity *prototype,
                                   const std::vector<JSEntity *> &args)
    : JSObjectEntity(prototype), _arguments(args) {
  _type = JSValueType::JS_ARGUMENT;
  for (auto &arg : args) {
    appendChild(arg);
  }
}
const std::vector<JSEntity *> &JSArgumentEntity::getArguments() const {
  return _arguments;
}