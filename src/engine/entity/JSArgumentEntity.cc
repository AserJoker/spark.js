#include "engine/entity/JSArgumentEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <fmt/xchar.h>
using namespace spark;
using namespace spark::engine;

JSArgumentEntity::JSArgumentEntity(JSStore *prototype,
                                   const std::vector<JSStore *> &args)
    : JSObjectEntity(prototype), _arguments(args) {
  _type = JSValueType::JS_ARGUMENT;
  size_t index = 0;
  for (auto &arg : args) {
    getProperties()[fmt::format(L"{}", index)] = {
        .configurable = true, .enumable = true, .value = arg, .writable = true};
    index++;
  }
}

const std::vector<JSStore *> &JSArgumentEntity::getArguments() const {
  return _arguments;
}