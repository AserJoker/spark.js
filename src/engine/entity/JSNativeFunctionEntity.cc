#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
#include <string>

using namespace spark;
using namespace spark::engine;

JSNativeFunctionEntity::JSNativeFunctionEntity(
    JSStore *funcProto, const std::wstring &name,
    const std::function<JSFunction> &callee,
    const common::Map<std::wstring, JSStore *> &closure)
    : JSObjectEntity(funcProto), _name(name), _callee(callee), _bind(nullptr),
      _closure(closure) {
  _type = JSValueType::JS_NATIVE_FUNCTION;
}

void JSNativeFunctionEntity::bind(JSStore *self) { _bind = self; }

const std::function<JSFunction> &JSNativeFunctionEntity::getCallee() const {
  return _callee;
};

const common::Map<std::wstring, JSStore *> &
JSNativeFunctionEntity::getClosure() const {
  return _closure;
}

const JSStore *
JSNativeFunctionEntity::getBind(common::AutoPtr<JSContext> ctx) const {
  return _bind;
}

JSStore *JSNativeFunctionEntity::getBind(common::AutoPtr<JSContext> ctx) {
  return _bind;
}

const std::wstring &JSNativeFunctionEntity::getFunctionName() const {
  static std::wstring anonymous = L"anonymous";
  if (_name.empty()) {
    return anonymous;
  }
  return _name;
}

std::wstring
JSNativeFunctionEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return fmt::format(L"function {}(){{[native code]}}", getFunctionName());
};

bool JSNativeFunctionEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
};
