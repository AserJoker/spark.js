#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;

JSFunctionEntity::JSFunctionEntity(
    JSEntity *funcProto, const std::wstring &name,
    const std::function<JSFunction> &callee,
    const common::Map<std::wstring, JSEntity *> &closure)
    : JSObjectEntity(funcProto), _name(name), _callee(callee),
      _prototype(nullptr), _bind(nullptr), _closure(closure) {
  _type = JSValueType::JS_FUNCTION;
}

void JSFunctionEntity::bind(JSEntity *self) { _bind = self; }

const std::function<JSFunction> &JSFunctionEntity::getCallee() const {
  return _callee;
};

const common::Map<std::wstring, JSEntity *> &
JSFunctionEntity::getClosure() const {
  return _closure;
}

const JSEntity *
JSFunctionEntity::getBind(common::AutoPtr<JSContext> ctx) const {
  return _bind;
}

void JSFunctionEntity::setPrototype(JSEntity *prototype) {
  _prototype = prototype;
}

const JSEntity *JSFunctionEntity::getPrototype() const { return _prototype; }

const std::wstring &JSFunctionEntity::getFunctionName() const {
  static std::wstring anonymous = L"anonymous";
  if (_name.empty()) {
    return anonymous;
  }
  return _name;
}

std::wstring JSFunctionEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return fmt::format(L"function {}(){{[native code]}}", getFunctionName());
};

bool JSFunctionEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
};
