#include "engine/runtime/JSContext.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/entity/JSBigIntEntity.hpp"
#include "engine/entity/JSBooleanEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/entity/JSNumberEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include <string>

using namespace spark;
using namespace spark::engine;
JSContext::JSContext(const common::AutoPtr<JSRuntime> &runtime)
    : _runtime(runtime) {
  _scope = new JSScope(_runtime->getRoot());
  _callStack = new JSFrame();
}
JSContext::~JSContext() {}

common::AutoPtr<JSRuntime> &JSContext::getRuntime() { return _runtime; }

JSScope *JSContext::pushScope() {
  auto scope = _scope;
  _scope = new JSScope(scope);
  return scope;
}

void JSContext::popScope(JSScope *scope) {
  while (_scope != scope) {
    auto now = _scope;
    _scope = _scope->getParent();
    delete now;
  }
}

void JSContext::setScope(JSScope *scope) { _scope = scope; }

JSScope *JSContext::getScope() { return _scope; }

void JSContext::pushCallStack(const std::wstring &funcname,
                              const JSLocation &location) {
  auto frame = new JSFrame;
  _callStack->location.filename = location.filename;
  _callStack->location.line = location.line;
  _callStack->location.column = location.column;
  frame->location.funcname = funcname;
  frame->parent = _callStack;
  _callStack = frame;
}

void JSContext::popCallStack() {
  auto frame = _callStack;
  _callStack = _callStack->parent;
  delete frame;
}

JSContext::JSFrame *JSContext::getCallStack() { return _callStack; }

common::Array<JSLocation> JSContext::trace(const JSLocation &location) {
  common::Array<JSLocation> stack;
  auto frame = _callStack;
  auto funcname = frame->location.funcname;
  if (funcname.empty()) {
    funcname = L"anonymous";
  }
  stack.pushBack({location.filename, location.line, location.column, funcname});
  frame = frame->parent;
  while (frame) {
    auto &[filename, line, column, fname] = frame->location;
    funcname = fname;
    if (funcname.empty()) {
      funcname = L"anonymous";
    }
    stack.pushBack({filename, line, column, funcname});
    frame = frame->parent;
  }
  return stack;
}

common::AutoPtr<JSValue> JSContext::createValue(JSEntity *entity,
                                                const std::wstring &name) {
  return _scope->createValue(entity, name);
}

common::AutoPtr<JSValue> JSContext::createValue(common::AutoPtr<JSValue> value,
                                                const std::wstring &name) {
  switch (value->getType()) {
  case JSValueType::JS_NUMBER:
    return createNumber(((JSNumberEntity *)(value->getEntity()))->getValue(),
                        name);
  case JSValueType::JS_STRING:
    return createString(((JSStringEntity *)(value->getEntity()))->getValue(),
                        name);
  case JSValueType::JS_BOOLEAN:
    return createBoolean(((JSBooleanEntity *)(value->getEntity()))->getValue(),
                         name);
  default:
    break;
  }
  return _scope->createValue(value->getEntity(), name);
}

common::AutoPtr<JSValue> JSContext::createNumber(double value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSNumberEntity(value), name);
}

common::AutoPtr<JSValue> JSContext::createString(const std::wstring &value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSStringEntity(value), name);
}

common::AutoPtr<JSValue> JSContext::createBoolean(bool value,
                                                  const std::wstring &name) {
  return _scope->createValue(new JSBooleanEntity(value), name);
}

common::AutoPtr<JSValue> JSContext::createBigInt(const common::BigInt<> &value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSBigIntEntity(value), name);
}

common::AutoPtr<JSValue> JSContext::createInfinity(bool negative,
                                                   const std::wstring &name) {
  return _scope->createValue(new JSInfinityEntity(negative), name);
}
common::AutoPtr<JSValue> JSContext::createObject(JSEntity *prototype,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSObjectEntity(prototype), name);
}

common::AutoPtr<JSValue>
JSContext::createFunction(const std::function<JSFunction> &value,
                          const std::wstring &funcname,
                          const std::wstring &name) {
  return _scope->createValue(new JSNativeFunctionEntity(funcname, value, {}),
                             name);
}

common::AutoPtr<JSValue>
JSContext::createFunction(const std::function<JSFunction> &value,
                          const common::Map<std::wstring, JSEntity *> closure,
                          const std::wstring &funcname,
                          const std::wstring &name) {
  return _scope->createValue(
      new JSNativeFunctionEntity(funcname, value, closure), name);
}

common::AutoPtr<JSValue>
JSContext::createException(const std::wstring &type,
                           const std::wstring &message,
                           const JSLocation &location) {
  return _scope->createValue(
      new JSExceptionEntity(type, message, trace(location)));
}

common::AutoPtr<JSValue> JSContext::Undefined() {
  return _runtime->Undefined();
}

common::AutoPtr<JSValue> JSContext::Null() { return _runtime->Null(); }

common::AutoPtr<JSValue> JSContext::NaN() { return _runtime->NaN(); }
