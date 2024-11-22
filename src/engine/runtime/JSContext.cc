#include "engine/runtime/JSContext.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/entity/JSBigIntEntity.hpp"
#include "engine/entity/JSBooleanEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/entity/JSNaNEntity.hpp"
#include "engine/entity/JSNullEntity.hpp"
#include "engine/entity/JSNumberEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "engine/entity/JSUndefinedEntity.hpp"
#include "engine/lib/JSFunctionConstructor.hpp"
#include "engine/lib/JSObjectConstructor.hpp"
#include "engine/lib/JSSymbolConstructor.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include <fstream>
#include <string>

using namespace spark;
using namespace spark::engine;

JS_FUNC(JSContext::JSArrayConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSErrorConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSNumberConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSStringConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSBooleanConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSBigIntConstructor) { return ctx->undefined(); }

JSContext::JSContext(const common::AutoPtr<JSRuntime> &runtime)
    : _runtime(runtime) {
  _scope = new JSScope(_runtime->getRoot());
  _root = _scope;
  _callStack = new JSFrame();
  initialize();
}

JSContext::~JSContext() {
  while (_callStack) {
    popCallStack();
  }
  delete _root;
}

void JSContext::initialize() {
  addRef();
  _undefined = _scope->createValue(new JSUndefinedEntity(), L"undefined");
  _null = _scope->createValue(new JSNullEntity(), L"null");
  _NaN = _scope->createValue(new JSNaNEntity(), L"NaN");
  _true = createBoolean(true);
  _false = createBoolean(false);

  auto objectPrototype = createValue(new JSObjectEntity(_null->getEntity()));
  auto functionPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto symbolPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto arrayPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto errorPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto numberPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto stringPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto booleanPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto bigintPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));
  auto regexpPrototype =
      createValue(new JSObjectEntity(objectPrototype->getEntity()));

  JSFunctionEntity *ObjectConstructorEntity =
      new JSFunctionEntity(functionPrototype->getEntity(), L"Object",
                           &JSObjectConstructor::constructor, {});
  _Object = _scope->createValue(ObjectConstructorEntity, L"Object");
  _Object->setProperty(this, L"prototype", objectPrototype);

  JSFunctionEntity *FunctionConstructorEntity =
      new JSFunctionEntity(functionPrototype->getEntity(), L"Function",
                           &JSFunctionConstructor::constructor, {});
  _Function = _scope->createValue(FunctionConstructorEntity, L"Function");
  _Function->setProperty(this, L"prototype", functionPrototype);

  JSFunctionEntity *SymbolConstructorEntity =
      new JSFunctionEntity(functionPrototype->getEntity(), L"Symbol",
                           &JSSymbolConstructor::constructor, {});
  _Symbol = _scope->createValue(SymbolConstructorEntity, L"Symbol");
  _Symbol->setProperty(this, L"prototype", symbolPrototype);

  JSFunctionEntity *ArrayConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"Array", &JSArrayConstructor, {});
  _Array = _scope->createValue(ArrayConstructorEntity, L"Array");
  _Array->setProperty(this, L"prototype", arrayPrototype);

  JSFunctionEntity *ErrorConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"Error", &JSErrorConstructor, {});
  _Error = _scope->createValue(ErrorConstructorEntity, L"Error");
  _Error->setProperty(this, L"prototype", errorPrototype);

  JSFunctionEntity *NumberConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"Number", &JSNumberConstructor, {});
  _Number = _scope->createValue(NumberConstructorEntity, L"Number");
  _Number->setProperty(this, L"prototype", numberPrototype);

  JSFunctionEntity *StringConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"String", &JSStringConstructor, {});
  _String = _scope->createValue(StringConstructorEntity, L"String");
  _String->setProperty(this, L"prototype", stringPrototype);

  JSFunctionEntity *BooleanConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"Boolean", &JSBooleanConstructor, {});
  _Boolean = _scope->createValue(BooleanConstructorEntity, L"Boolean");
  _Boolean->setProperty(this, L"prototype", booleanPrototype);

  JSFunctionEntity *BigIntConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"BigInt", &JSBigIntConstructor, {});
  _BigInt = _scope->createValue(BigIntConstructorEntity, L"BigInt");
  _BigInt->setProperty(this, L"prototype", bigintPrototype);

  JSFunctionEntity *RegExpConstructorEntity = new JSFunctionEntity(
      functionPrototype->getEntity(), L"RegExp", &JSBigIntConstructor, {});
  _RegExp = _scope->createValue(BigIntConstructorEntity, L"RegExp");
  _RegExp->setProperty(this, L"prototype", regexpPrototype);

  _symbolValue = createSymbol();
  _symbolPack = createSymbol();

  auto scope = pushScope();
  JSSymbolConstructor::initialize(this, _Symbol);
  JSObjectConstructor::initialize(this, _Object);
  JSFunctionConstructor::initialize(this, _Function);
  popScope(scope);
  subRef();
}

common::AutoPtr<JSRuntime> &JSContext::getRuntime() { return _runtime; }

common::AutoPtr<JSValue> JSContext::eval(const std::wstring &source,
                                         const std::wstring &filename) {
  auto parser = _runtime->getParser();
  auto index = _runtime->setSourceFilename(filename);
  auto ast = parser->parse(index, source);
  std::wofstream out("1.json");
  out << parser->toJSON(filename, source, ast);
  return _undefined;
}

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
JSScope *JSContext::getRoot() { return _root; }

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

std::vector<JSLocation> JSContext::trace(const JSLocation &location) {
  std::vector<JSLocation> stack;
  auto frame = _callStack;
  auto funcname = frame->location.funcname;
  if (funcname.empty()) {
    funcname = L"anonymous";
  }
  stack.push_back(
      {location.filename, location.line, location.column, funcname});
  frame = frame->parent;
  while (frame) {
    auto &[filename, line, column, fname] = frame->location;
    funcname = fname;
    if (funcname.empty()) {
      funcname = L"anonymous";
    }
    stack.push_back({filename, line, column, funcname});
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
  case JSValueType::JS_BIGINT:
    return createBigInt(value->getEntity<JSBigIntEntity>()->getValue(), name);
  case JSValueType::JS_INFINITY:
    return createInfinity(value->getEntity<JSInfinityEntity>()->isNegative(),
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
common::AutoPtr<JSValue> JSContext::createSymbol(const std::wstring &value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSSymbolEntity(value), name);
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
common::AutoPtr<JSValue>
JSContext::createObject(common::AutoPtr<JSValue> prototype,
                        const std::wstring &name) {
  JSEntity *proto = nullptr;
  if (prototype != nullptr) {
    proto = prototype->getEntity();
  } else {
    proto = _Object->getProperty(this, L"prototype")->getEntity();
  }
  return _scope->createValue(new JSObjectEntity(proto), name);
}
common::AutoPtr<JSValue> JSContext::createObject(const std::wstring &name) {
  return createObject(nullptr, name);
}

common::AutoPtr<JSValue>
JSContext::createFunction(const std::function<JSFunction> &value,
                          const std::wstring &funcname,
                          const std::wstring &name) {
  return _scope->createValue(
      new JSFunctionEntity(
          _Function->getProperty(this, L"prototype")->getEntity(), funcname,
          value, {}),
      name);
}

common::AutoPtr<JSValue>
JSContext::createFunction(const std::function<JSFunction> &value,
                          const common::Map<std::wstring, JSEntity *> closure,
                          const std::wstring &funcname,
                          const std::wstring &name) {
  return _scope->createValue(
      new JSFunctionEntity(
          _Function->getProperty(this, L"prototype")->getEntity(), funcname,
          value, closure),
      name);
}

common::AutoPtr<JSValue>
JSContext::createException(const std::wstring &type,
                           const std::wstring &message,
                           const JSLocation &location) {
  return _scope->createValue(new JSExceptionEntity(
      _Error->getProperty(this, L"prototype")->getEntity(), type, message,
      trace(location)));
}

common::AutoPtr<JSValue> JSContext::undefined() { return _undefined; }

common::AutoPtr<JSValue> JSContext::null() { return _null; }

common::AutoPtr<JSValue> JSContext::NaN() { return _NaN; }

common::AutoPtr<JSValue> JSContext::Symbol() { return _Symbol; }

common::AutoPtr<JSValue> JSContext::truly() { return _true; }

common::AutoPtr<JSValue> JSContext::falsely() { return _false; }

common::AutoPtr<JSValue> JSContext::symbolValue() { return _symbolValue; }

common::AutoPtr<JSValue> JSContext::symbolPack() { return _symbolPack; }