#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"

using namespace spark;
using namespace spark::engine;
JSFunctionEntity::JSFunctionEntity(
    JSEntity *prototype, const common::AutoPtr<compiler::JSModule> &module)
    : JSObjectEntity(prototype), _async(false), _generator(false), _address(0),
      _length(0) {
  _type = JSValueType::JS_FUNCTION;
  _module = module;
}
void JSFunctionEntity::setAsync(bool async) { _async = async; }
void JSFunctionEntity::setGenerator(bool generator) { _generator = generator; }

void JSFunctionEntity::setAddress(uint32_t address) { _address = address; }

void JSFunctionEntity::setLength(uint32_t length) { _length = length; }

void JSFunctionEntity::setClosure(const std::wstring &name, JSEntity *entity) {
  if (_closure.contains(name)) {
    if (_closure.at(name) == entity) {
      return;
    }
    appendChild(entity);
    removeChild(_closure.at(name));
  } else {
    appendChild(entity);
  }
  _closure[name] = entity;
}
void JSFunctionEntity::setSource(const std::wstring &source) {
  _source = source;
}
void JSFunctionEntity::setFuncName(const std::wstring &name) { _name = name; }
bool JSFunctionEntity::getAsync() const { return _async; }
bool JSFunctionEntity::getGenerator() const { return _generator; }
uint32_t JSFunctionEntity::getAddress() const { return _address; }
uint32_t JSFunctionEntity::getLength() const { return _length; }
const std::wstring &JSFunctionEntity::getFuncName() const { return _name; }

const std::wstring &JSFunctionEntity::getFunctionSource() const {
  return _source;
}

const std::unordered_map<std::wstring, JSEntity *> &
JSFunctionEntity::getClosure() const {
  return _closure;
}
const common::AutoPtr<compiler::JSModule> &JSFunctionEntity::getModule() const {
  return _module;
}