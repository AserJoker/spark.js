#include "engine/entity/JSArgumentEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;

JSArgumentEntity::JSArgumentEntity(JSEntity *prototype,
                                   const std::vector<JSEntity *> &args)
    : JSObjectEntity(prototype), _arguments(args) {
  _type = JSValueType::JS_ARGUMENT;
  size_t index = 0;
  for (auto &arg : args) {
    appendChild(arg);
    getProperties()[fmt::format(L"{}", index)] = {
        .configurable = true, .enumable = true, .value = arg, .writable = true};
    index++;
  }
}

const std::vector<JSEntity *> &JSArgumentEntity::getArguments() const {
  return _arguments;
}