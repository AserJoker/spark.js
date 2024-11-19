#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;

JSNativeFunctionEntity::JSNativeFunctionEntity(
    const std::wstring &name, const std::function<JSFunction> &callee,
    const common::Map<std::wstring, JSEntity *> &closure)
    : JSEntity(JSValueType::JS_FUNCTION), _name(name), _callee(callee),
      _bind(nullptr), _closure(closure) {}

void JSNativeFunctionEntity::bind(common::AutoPtr<JSContext> ctx,
                                  JSEntity *self) {
  if (_bind == self) {
    return;
  }
  auto old = _bind;
  _bind = self;
  if (_bind != nullptr) {
    appendChild(_bind);
  }
  if (old != nullptr) {
    removeChild(old);
    ctx->getScope()->getRoot()->appendChild(old);
  }
}

const std::function<JSFunction> &JSNativeFunctionEntity::getCallee() const {
  return _callee;
};

const common::Map<std::wstring, JSEntity *> &
JSNativeFunctionEntity::getClosure() const {
  return _closure;
}

const JSEntity *
JSNativeFunctionEntity::getBind(common::AutoPtr<JSContext> ctx) const {
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
