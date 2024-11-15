#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/runtime/JSContext.hpp"
#include <fmt/xchar.h>
#include <string>
using namespace spark;
using namespace spark::engine;

JSNativeFunctionEntity::JSNativeFunctionEntity(
    const std::wstring &name, const std::function<JSFunction> &callee,
    const common::Map<std::wstring, JSEntity *> &closure)
    : JSBaseEntity(JSValueType::JS_NATIVE_FUNCTION,
                   JSNativeFunctionData{name, callee, nullptr, closure}) {}

common::AutoPtr<JSValue>
JSNativeFunctionEntity::apply(common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> self,
                              common::Array<common::AutoPtr<JSValue>> args) {
  auto &[_, callee, bind, closure] = getData();
  for (auto &[name, entity] : closure) {
    ctx->createValue(entity, name);
  }
  return callee(ctx, bind != nullptr ? ctx->createValue(bind) : self, args);
}

void JSNativeFunctionEntity::bind(common::AutoPtr<JSValue> self) {
  auto data = getData();
  if (data.bind == nullptr) {
    data.bind = self->getEntity();
    appendChild(data.bind);
  }
}

const std::function<JSFunction> &JSNativeFunctionEntity::getCallee() const {
  return getData().callee;
};

const common::Map<std::wstring, JSEntity *> &
JSNativeFunctionEntity::getClosure() const {
  return getData().closure;
}

const JSEntity *
JSNativeFunctionEntity::getBind(common::AutoPtr<JSContext> ctx) const {
  return getData().bind;
}

const std::wstring &JSNativeFunctionEntity::getFunctionName() const {
  static std::wstring anonymous = L"anonymous";
  auto &name = getData().name;
  if (name.empty()) {
    return anonymous;
  }
  return name;
}

std::wstring
JSNativeFunctionEntity::toString(common::AutoPtr<JSContext> ctx) const {
  return fmt::format(L"function {}(){{[native code]}}", getFunctionName());
};

bool JSNativeFunctionEntity::toBoolean(common::AutoPtr<JSContext> ctx) const {
  return true;
};

std::wstring
JSNativeFunctionEntity::getTypeName(common::AutoPtr<JSContext> ctx) const {
  return L"function";
};