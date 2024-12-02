#include "engine/runtime/JSContext.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSBigIntEntity.hpp"
#include "engine/entity/JSBooleanEntity.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSExceptionEntity.hpp"
#include "engine/entity/JSFunctionEntity.hpp"
#include "engine/entity/JSInfinityEntity.hpp"
#include "engine/entity/JSNaNEntity.hpp"
#include "engine/entity/JSNativeFunctionEntity.hpp"
#include "engine/entity/JSNullEntity.hpp"
#include "engine/entity/JSNumberEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "engine/entity/JSUndefinedEntity.hpp"
#include "engine/lib/JSErrorConstructor.hpp"
#include "engine/lib/JSFunctionConstructor.hpp"
#include "engine/lib/JSObjectConstructor.hpp"
#include "engine/lib/JSSymbolConstructor.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSSyntaxError.hpp"
#include <string>

using namespace spark;
using namespace spark::engine;

JS_FUNC(JSContext::JSArrayConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSNumberConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSStringConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSBooleanConstructor) { return ctx->undefined(); }
JS_FUNC(JSContext::JSBigIntConstructor) { return ctx->undefined(); }

JSContext::JSContext(const common::AutoPtr<JSRuntime> &runtime)
    : _runtime(runtime) {
  _scope = new JSScope(_runtime->getRoot());
  _root = _scope;
  _callStack = new JSCallFrame();
  initialize();
  createValue(_undefined, L"this");
}

JSContext::~JSContext() {
  while (_callStack) {
    popCallStack();
  }
}

void JSContext::initialize() {
  addRef();
  _undefined = _scope->createValue(new JSUndefinedEntity(), L"undefined");
  _null = _scope->createValue(new JSNullEntity(), L"null");
  _NaN = _scope->createValue(new JSNaNEntity(), L"NaN");
  _true = createBoolean(true);
  _false = createBoolean(false);
  _uninitialized =
      _scope->createValue(new JSEntity(JSValueType::JS_UNINITIALIZED));

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

  JSNativeFunctionEntity *ObjectConstructorEntity =
      new JSNativeFunctionEntity(functionPrototype->getEntity(), L"Object",
                                 &JSObjectConstructor::constructor, {});
  _Object = _scope->createValue(ObjectConstructorEntity, L"Object");
  _Object->setProperty(this, L"prototype", objectPrototype);
  objectPrototype->setProperty(this, L"constructor", _Object);

  JSNativeFunctionEntity *FunctionConstructorEntity =
      new JSNativeFunctionEntity(functionPrototype->getEntity(), L"Function",
                                 &JSFunctionConstructor::constructor, {});
  _Function = _scope->createValue(FunctionConstructorEntity, L"Function");
  _Function->setProperty(this, L"prototype", functionPrototype);
  functionPrototype->setProperty(this, L"constructor", _Function);

  JSNativeFunctionEntity *SymbolConstructorEntity =
      new JSNativeFunctionEntity(functionPrototype->getEntity(), L"Symbol",
                                 &JSSymbolConstructor::constructor, {});
  _Symbol = _scope->createValue(SymbolConstructorEntity, L"Symbol");
  _Symbol->setProperty(this, L"prototype", symbolPrototype);
  symbolPrototype->setProperty(this, L"constructor", _Symbol);

  JSNativeFunctionEntity *ArrayConstructorEntity = new JSNativeFunctionEntity(
      functionPrototype->getEntity(), L"Array", &JSArrayConstructor, {});
  _Array = _scope->createValue(ArrayConstructorEntity, L"Array");
  _Array->setProperty(this, L"prototype", arrayPrototype);
  arrayPrototype->setProperty(this, L"constructor", _Array);

  JSNativeFunctionEntity *ErrorConstructorEntity =
      new JSNativeFunctionEntity(functionPrototype->getEntity(), L"Error",
                                 &JSErrorConstructor::constructor, {});
  _Error = _scope->createValue(ErrorConstructorEntity, L"Error");
  _Error->setProperty(this, L"prototype", errorPrototype);
  errorPrototype->setProperty(this, L"constructor", _Error);

  JSNativeFunctionEntity *NumberConstructorEntity = new JSNativeFunctionEntity(
      functionPrototype->getEntity(), L"Number", &JSNumberConstructor, {});
  _Number = _scope->createValue(NumberConstructorEntity, L"Number");
  _Number->setProperty(this, L"prototype", numberPrototype);
  numberPrototype->setProperty(this, L"constructor", _Number);

  JSNativeFunctionEntity *StringConstructorEntity = new JSNativeFunctionEntity(
      functionPrototype->getEntity(), L"String", &JSStringConstructor, {});
  _String = _scope->createValue(StringConstructorEntity, L"String");
  _String->setProperty(this, L"prototype", stringPrototype);
  stringPrototype->setProperty(this, L"constructor", _String);

  JSNativeFunctionEntity *BooleanConstructorEntity = new JSNativeFunctionEntity(
      functionPrototype->getEntity(), L"Boolean", &JSBooleanConstructor, {});
  _Boolean = _scope->createValue(BooleanConstructorEntity, L"Boolean");
  _Boolean->setProperty(this, L"prototype", booleanPrototype);
  booleanPrototype->setProperty(this, L"constructor", _Boolean);

  JSNativeFunctionEntity *BigIntConstructorEntity = new JSNativeFunctionEntity(
      functionPrototype->getEntity(), L"BigInt", &JSBigIntConstructor, {});
  _BigInt = _scope->createValue(BigIntConstructorEntity, L"BigInt");
  _BigInt->setProperty(this, L"prototype", bigintPrototype);
  bigintPrototype->setProperty(this, L"constructor", _BigInt);

  JSNativeFunctionEntity *RegExpConstructorEntity = new JSNativeFunctionEntity(
      functionPrototype->getEntity(), L"RegExp", &JSBigIntConstructor, {});
  _RegExp = _scope->createValue(RegExpConstructorEntity, L"RegExp");
  _RegExp->setProperty(this, L"prototype", regexpPrototype);
  regexpPrototype->setProperty(this, L"constructor", _RegExp);

  _symbolValue = createSymbol();
  _symbolPack = createSymbol();

  auto scope = pushScope();
  JSSymbolConstructor::initialize(this, _Symbol, symbolPrototype);
  JSObjectConstructor::initialize(this, _Object, objectPrototype);
  JSFunctionConstructor::initialize(this, _Function, functionPrototype);
  JSErrorConstructor::initialize(this, _Error, errorPrototype);
  popScope(scope);
  subRef();
}

common::AutoPtr<JSRuntime> &JSContext::getRuntime() { return _runtime; }

common::AutoPtr<JSValue> JSContext::eval(const std::wstring &source,
                                         const std::wstring &filename) {
  auto module = compile(source, filename);
  return _runtime->getVirtualMachine()->eval(this, module);
}

common::AutoPtr<compiler::JSModule>
JSContext::compile(const std::wstring &source, const std::wstring &filename) {
  auto parser = _runtime->getParser();
  auto generator = _runtime->getGenerator();
  auto index = _runtime->setSourceFilename(filename);
  auto ast = parser->parse(index, source);
  return generator->resolve(filename, source, ast);
}

common::AutoPtr<JSScope> JSContext::pushScope() {
  auto scope = _scope;
  _scope = new JSScope(scope);
  return scope;
}

void JSContext::popScope(common::AutoPtr<JSScope> scope) {
  if (_scope != scope) {
    while (_scope->getParent() != scope) {
      auto parent = _scope->getParent();
      _scope = parent;
    }
    auto parent = _scope->getParent();
    parent->removeChild(_scope);
    _scope = scope;
  }
}

void JSContext::setScope(JSScope *scope) { _scope = scope; }

common::AutoPtr<JSScope> JSContext::getScope() { return _scope; }
common::AutoPtr<JSScope> JSContext::getRoot() { return _root; }

void JSContext::pushCallStack(const JSLocation &location) {
  auto frame = new JSCallFrame;
  _callStack->location.filename = location.filename;
  _callStack->location.line = location.line;
  _callStack->location.column = location.column;
  frame->location.funcname = location.funcname;
  frame->parent = _callStack;
  _callStack = frame;
}

void JSContext::popCallStack() {
  auto frame = _callStack;
  _callStack = _callStack->parent;
  delete frame;
}

JSContext::JSCallFrame *JSContext::getCallStack() { return _callStack; }

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
JSContext::constructObject(common::AutoPtr<JSValue> constructor,
                           const std::vector<common::AutoPtr<JSValue>> &args,
                           const JSLocation &loc, const std::wstring &name) {
  if (constructor->getType() != JSValueType::JS_FUNCTION &&
      constructor->getType() != JSValueType::JS_NATIVE_FUNCTION &&
      constructor->getType() != JSValueType::JS_CLASS) {
    throw error::JSSyntaxError(
        fmt::format(L"'{}' is not a constructor", constructor->getName()));
  }
  auto prototype = constructor->getProperty(this, L"prototype");
  auto result = createObject(prototype, name);
  result->setProperty(this, L"constructor", constructor);
  constructor->apply(this, result, args, loc);
  return result;
}

common::AutoPtr<JSValue>
JSContext::createNativeFunction(const std::function<JSFunction> &value,
                                const std::wstring &funcname,
                                const std::wstring &name) {
  auto entity = new JSNativeFunctionEntity(
      _Function->getProperty(this, L"prototype")->getEntity(), funcname, value,
      {});
  return _scope->createValue(entity, name);
}

common::AutoPtr<JSValue> JSContext::createNativeFunction(
    const std::function<JSFunction> &value,
    const common::Map<std::wstring, JSEntity *> closure,
    const std::wstring &funcname, const std::wstring &name) {
  return _scope->createValue(
      new JSNativeFunctionEntity(
          _Function->getProperty(this, L"prototype")->getEntity(), funcname,
          value, closure),
      name);
}
common::AutoPtr<JSValue>
JSContext::createFunction(const common::AutoPtr<compiler::JSModule> &module,
                          const std::wstring &name) {
  return _scope->createValue(
      new JSFunctionEntity(
          _Function->getProperty(this, L"prototype")->getEntity(), module),
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
common::AutoPtr<JSValue>
JSContext::createException(common::AutoPtr<JSValue> target) {
  return _scope->createValue(new JSExceptionEntity(
      _Error->getProperty(this, L"prototype")->getEntity(),
      target->getEntity()));
}

common::AutoPtr<JSValue> JSContext::undefined() { return _undefined; }

common::AutoPtr<JSValue> JSContext::null() { return _null; }

common::AutoPtr<JSValue> JSContext::NaN() { return _NaN; }

common::AutoPtr<JSValue> JSContext::Symbol() { return _Symbol; }

common::AutoPtr<JSValue> JSContext::truly() { return _true; }

common::AutoPtr<JSValue> JSContext::falsely() { return _false; }

common::AutoPtr<JSValue> JSContext::symbolValue() { return _symbolValue; }

common::AutoPtr<JSValue> JSContext::symbolPack() { return _symbolPack; }

common::AutoPtr<JSValue> JSContext::uninitialized() { return _uninitialized; }

common::AutoPtr<JSValue> JSContext::load(const std::wstring &name) {
  auto val = _scope->getValue(name);
  if (!val) {
    throw error::JSSyntaxError(fmt::format(L"'{}' is not defined", name));
  }
  return val;
}