#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;

JSExceptionEntity::JSExceptionEntity(JSEntity *prototype,
                                     const std::wstring &type,
                                     const std::wstring &message,
                                     const std::vector<JSLocation> &stack)
    : JSObjectEntity(prototype), _errorType(type), _message(message),
      _stack(stack), _target(nullptr) {
  _type = JSValueType::JS_EXCEPTION;
}

JSExceptionEntity::JSExceptionEntity(JSEntity *prototype, JSEntity *target)
    : JSObjectEntity(prototype), _target(target) {
  appendChild(target);
  _type = JSValueType::JS_EXCEPTION;
}

JSEntity *JSExceptionEntity::getTarget() { return _target; }

const std::wstring &JSExceptionEntity::getMessage() const { return _message; }

const std::wstring &JSExceptionEntity::getExceptionType() const {
  return _errorType;
}

const std::vector<JSLocation> &JSExceptionEntity::getStack() const {
  return _stack;
}

bool JSExceptionEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
}