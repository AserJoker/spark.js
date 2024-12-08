#include "engine/runtime/JSContext.hpp"
#include "common/AutoPtr.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSArrayEntity.hpp"
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
#include "engine/lib/JSArrayConstructor.hpp"
#include "engine/lib/JSErrorConstructor.hpp"
#include "engine/lib/JSFunctionConstructor.hpp"
#include "engine/lib/JSGeneratorConstructor.hpp"
#include "engine/lib/JSGeneratorFunctionConstructor.hpp"
#include "engine/lib/JSIteratorConstructor.hpp"
#include "engine/lib/JSObjectConstructor.hpp"
#include "engine/lib/JSSymbolConstructor.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSStore.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSSyntaxError.hpp"
#include "error/JSTypeError.hpp"
#include <string>

using namespace spark;
using namespace spark::engine;

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
  _undefined =
      _scope->createValue(new JSStore(new JSUndefinedEntity()), L"undefined");
  _null = _scope->createValue(new JSStore(new JSNullEntity()), L"null");
  _NaN = _scope->createValue(new JSStore(new JSNaNEntity()), L"NaN");
  _true = createBoolean(true);
  _false = createBoolean(false);
  _uninitialized = _scope->createValue(
      new JSStore(new JSEntity(JSValueType::JS_UNINITIALIZED)));

  auto objectPrototype =
      createValue(new JSStore(new JSObjectEntity(_null->getStore())));
  objectPrototype->getStore()->appendChild(_null->getStore());

  auto functionPrototype =
      createValue(new JSStore(new JSObjectEntity(objectPrototype->getStore())));

  functionPrototype->getStore()->appendChild(objectPrototype->getStore());

  JSStore *ObjectConstructorEntity = new JSStore(
      new JSNativeFunctionEntity(functionPrototype->getStore(), L"Object",
                                 &JSObjectConstructor::constructor, {}));
  ObjectConstructorEntity->appendChild(functionPrototype->getStore());
  _Object = _scope->createValue(ObjectConstructorEntity, L"Object");
  _Object->setProperty(this, L"prototype", objectPrototype);
  objectPrototype->setProperty(this, L"constructor", _Object);

  JSStore *FunctionConstructorEntity = new JSStore(
      new JSNativeFunctionEntity(functionPrototype->getStore(), L"Function",
                                 &JSFunctionConstructor::constructor, {}));
  FunctionConstructorEntity->appendChild(functionPrototype->getStore());
  _Function = _scope->createValue(FunctionConstructorEntity, L"Function");
  _Function->setProperty(this, L"prototype", functionPrototype);
  functionPrototype->setProperty(this, L"constructor", _Function);

  auto symbolPrototype = createObject();
  _Symbol = createNativeFunction(JSSymbolConstructor::constructor, L"Symbol",
                                 L"Symbol");
  _Symbol->setProperty(this, L"prototype", symbolPrototype);
  symbolPrototype->setProperty(this, L"constructor", _Symbol);

  _symbolValue = createSymbol();
  _symbolPack = createSymbol();

  JSSymbolConstructor::initialize(this, _Symbol, symbolPrototype);
  JSObjectConstructor::initialize(this, _Object, objectPrototype);
  JSFunctionConstructor::initialize(this, _Function, functionPrototype);
  _Error = JSErrorConstructor::initialize(this);
  _Array = JSArrayConstructor::initialize(this);
  _GeneratorFunction = JSGeneratorFunctionConstructor::initialize(this);
  _Iterator = JSIteratorConstructor::initialize(this);
  _Generator = JSGeneratorConstructor::initialize(this);
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

void JSContext::pushScope() { _scope = new JSScope(_scope); }

void JSContext::popScope() {
  auto parent = _scope->getParent();
  parent->removeChild(_scope);
  _scope = parent;
}

common::AutoPtr<JSScope> JSContext::setScope(common::AutoPtr<JSScope> scope) {
  auto result = _scope;
  _scope = scope;
  return result;
}

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

common::AutoPtr<JSValue> JSContext::createValue(JSStore *store,
                                                const std::wstring &name) {
  return _scope->createValue(store, name);
}

common::AutoPtr<JSValue> JSContext::createValue(common::AutoPtr<JSValue> value,
                                                const std::wstring &name) {
  switch (value->getType()) {
  case JSValueType::JS_NUMBER:
    return createNumber(value->getEntity<JSNumberEntity>()->getValue(), name);
  case JSValueType::JS_STRING:
    return createString(value->getEntity<JSStringEntity>()->getValue(), name);
  case JSValueType::JS_BOOLEAN:
    return createBoolean(value->getEntity<JSBooleanEntity>()->getValue(), name);
  case JSValueType::JS_BIGINT:
    return createBigInt(value->getEntity<JSBigIntEntity>()->getValue(), name);
  case JSValueType::JS_INFINITY:
    return createInfinity(value->getEntity<JSInfinityEntity>()->isNegative(),
                          name);
  default:
    break;
  }
  return _scope->createValue(value->getStore(), name);
}

common::AutoPtr<JSValue> JSContext::createNumber(double value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSStore(new JSNumberEntity(value)), name);
}

common::AutoPtr<JSValue> JSContext::createString(const std::wstring &value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSStore(new JSStringEntity(value)), name);
}
common::AutoPtr<JSValue> JSContext::createSymbol(const std::wstring &value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSStore(new JSSymbolEntity(value)), name);
}

common::AutoPtr<JSValue> JSContext::createBoolean(bool value,
                                                  const std::wstring &name) {
  return _scope->createValue(new JSStore(new JSBooleanEntity(value)), name);
}

common::AutoPtr<JSValue> JSContext::createBigInt(const common::BigInt<> &value,
                                                 const std::wstring &name) {
  return _scope->createValue(new JSStore(new JSBigIntEntity(value)), name);
}

common::AutoPtr<JSValue> JSContext::createInfinity(bool negative,
                                                   const std::wstring &name) {
  return _scope->createValue(new JSStore(new JSInfinityEntity(negative)), name);
}
common::AutoPtr<JSValue>
JSContext::createObject(common::AutoPtr<JSValue> prototype,
                        const std::wstring &name) {
  JSStore *proto = nullptr;
  if (prototype != nullptr) {
    proto = prototype->getStore();
  } else {
    proto = _Object->getProperty(this, L"prototype")->getStore();
  }
  auto res = _scope->createValue(new JSStore(new JSObjectEntity(proto)), name);
  res->getStore()->appendChild(proto);
  return res;
}

common::AutoPtr<JSValue> JSContext::createObject(const std::wstring &name) {
  return createObject(nullptr, name);
}

common::AutoPtr<JSValue> JSContext::createArray(const std::wstring &name) {
  return constructObject(_Array, {}, {});
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
  common::AutoPtr<JSValue> result;
  if (constructor == _Array) {
    result = createValue(new JSStore(new JSArrayEntity(prototype->getStore())));
    result->getStore()->appendChild(prototype->getStore());
  } else if (constructor == _Function) {
    result = createValue(
        new JSStore(new JSFunctionEntity(prototype->getStore(), nullptr)));
    result->getStore()->appendChild(prototype->getStore());
  } else if (constructor == _Symbol) {
    throw error::JSTypeError(L"Symbol is not a constructor");
  } else {
    result = createObject(prototype, name);
  }
  result->getStore()->appendChild(prototype->getStore());
  result->setProperty(this, L"constructor", constructor);
  constructor->apply(this, result, args, loc);
  return result;
}

common::AutoPtr<JSValue>
JSContext::createNativeFunction(const std::function<JSFunction> &value,
                                const std::wstring &funcname,
                                const std::wstring &name) {
  auto prop = _Function->getProperty(this, L"prototype")->getStore();
  auto store =
      new JSStore(new JSNativeFunctionEntity(prop, funcname, value, {}));
  store->appendChild(prop);
  return _scope->createValue(store, name);
}

common::AutoPtr<JSValue> JSContext::createNativeFunction(
    const std::function<JSFunction> &value,
    const common::Map<std::wstring, JSStore *> closure,
    const std::wstring &funcname, const std::wstring &name) {
  auto prop = _Function->getProperty(this, L"prototype")->getStore();
  auto res = _scope->createValue(
      new JSStore(new JSNativeFunctionEntity(prop, funcname, value, closure)),
      name);
  res->getStore()->appendChild(prop);
  return res;
}
common::AutoPtr<JSValue>
JSContext::createFunction(const common::AutoPtr<compiler::JSModule> &module,
                          const std::wstring &name) {
  auto prop = _Function->getProperty(this, L"prototype")->getStore();
  auto res = _scope->createValue(
      new JSStore(new JSFunctionEntity(prop, module)), name);
  res->getStore()->appendChild(prop);
  return res;
}
common::AutoPtr<JSValue>
JSContext::createGenerator(const common::AutoPtr<compiler::JSModule> &module,
                           const std::wstring &name) {
  auto prop = _GeneratorFunction->getProperty(this, L"prototype")->getStore();
  auto store = new JSStore(new JSFunctionEntity(prop, module));
  auto res = _scope->createValue(store, name);
  res->getEntity<JSFunctionEntity>()->setGenerator(true);
  res->getStore()->appendChild(prop);
  return res;
}

common::AutoPtr<JSValue>
JSContext::createException(const std::wstring &type,
                           const std::wstring &message,
                           const JSLocation &location) {
  return _scope->createValue(
      new JSStore(new JSExceptionEntity(type, message, trace(location))));
}
common::AutoPtr<JSValue>
JSContext::createException(common::AutoPtr<JSValue> target) {
  auto res = _scope->createValue(
      new JSStore(new JSExceptionEntity(target->getStore())));
  res->getStore()->appendChild(target->getStore());
  return res;
}

common::AutoPtr<JSValue> JSContext::undefined() { return _undefined; }

common::AutoPtr<JSValue> JSContext::null() { return _null; }

common::AutoPtr<JSValue> JSContext::NaN() { return _NaN; }

common::AutoPtr<JSValue> JSContext::truly() { return _true; }

common::AutoPtr<JSValue> JSContext::falsely() { return _false; }

common::AutoPtr<JSValue> JSContext::Symbol() { return _Symbol; }

common::AutoPtr<JSValue> JSContext::Function() { return _Function; }

common::AutoPtr<JSValue> JSContext::Object() { return _Object; }

common::AutoPtr<JSValue> JSContext::Error() { return _Error; }

common::AutoPtr<JSValue> JSContext::Array() { return _Array; }

common::AutoPtr<JSValue> JSContext::Iterator() { return _Iterator; }

common::AutoPtr<JSValue> JSContext::GeneratorFunction() {
  return _GeneratorFunction;
}

common::AutoPtr<JSValue> JSContext::Generator() { return _Generator; }

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