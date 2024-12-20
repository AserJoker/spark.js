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
#include "engine/entity/JSPromiseEntity.hpp"
#include "engine/entity/JSStringEntity.hpp"
#include "engine/entity/JSSymbolEntity.hpp"
#include "engine/entity/JSUndefinedEntity.hpp"
#include "engine/lib/JSAggregateErrorConstructor.hpp"
#include "engine/lib/JSArrayConstructor.hpp"
#include "engine/lib/JSErrorConstructor.hpp"
#include "engine/lib/JSFunctionConstructor.hpp"
#include "engine/lib/JSGeneratorConstructor.hpp"
#include "engine/lib/JSGeneratorFunctionConstructor.hpp"
#include "engine/lib/JSInternalErrorConstructor.hpp"
#include "engine/lib/JSIteratorConstructor.hpp"
#include "engine/lib/JSObjectConstructor.hpp"
#include "engine/lib/JSPromiseConstructor.hpp"
#include "engine/lib/JSRangeErrorConstructor.hpp"
#include "engine/lib/JSReferenceErrorConstructor.hpp"
#include "engine/lib/JSSymbolConstructor.hpp"
#include "engine/lib/JSSyntaxErrorConstructor.hpp"
#include "engine/lib/JSTypeErrorConstructor.hpp"
#include "engine/lib/JSURIErrorConstructor.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSStore.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSSyntaxError.hpp"
#include "error/JSTypeError.hpp"
#include "vm/JSCoroutineContext.hpp"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace spark;
using namespace spark::engine;

JSContext::JSContext(const common::AutoPtr<JSRuntime> &runtime)
    : _runtime(runtime) {
  _scope = new JSScope(_runtime->getRoot());
  _root = _scope;
  _callStack = new JSCallFrame();
  initialize();
}

JSContext::~JSContext() {
  while (_callStack) {
    popCallStack();
  }
}

void JSContext::initialize() {
  addRef();
  auto null = this->null();
  auto objectPrototype =
      createValue(new JSStore(new JSObjectEntity(null->getStore())));
  objectPrototype->getStore()->appendChild(null->getStore());

  auto functionPrototype =
      createValue(new JSStore(new JSObjectEntity(objectPrototype->getStore())));

  functionPrototype->getStore()->appendChild(objectPrototype->getStore());

  JSStore *ObjectConstructorEntity = new JSStore(
      new JSNativeFunctionEntity(functionPrototype->getStore(), L"Object",
                                 &JSObjectConstructor::constructor, {}));
  ObjectConstructorEntity->appendChild(functionPrototype->getStore());
  _Object = _scope->createValue(ObjectConstructorEntity, L"Object");
  _Object->setPropertyDescriptor(this, L"prototype", objectPrototype);
  objectPrototype->setPropertyDescriptor(this, L"constructor", _Object);

  JSStore *FunctionConstructorEntity = new JSStore(
      new JSNativeFunctionEntity(functionPrototype->getStore(), L"Function",
                                 &JSFunctionConstructor::constructor, {}));
  FunctionConstructorEntity->appendChild(functionPrototype->getStore());
  _Function = _scope->createValue(FunctionConstructorEntity, L"Function");
  _Function->setPropertyDescriptor(this, L"prototype", functionPrototype);
  functionPrototype->setPropertyDescriptor(this, L"constructor", _Function);

  auto symbolPrototype = createObject();
  _Symbol = createNativeFunction(JSSymbolConstructor::constructor, L"Symbol",
                                 L"Symbol");
  _Symbol->setPropertyDescriptor(this, L"prototype", symbolPrototype);
  symbolPrototype->setPropertyDescriptor(this, L"constructor", _Symbol);

  _symbolValue = createSymbol();
  _symbolPack = createSymbol();

  JSSymbolConstructor::initialize(this, _Symbol, symbolPrototype);
  JSObjectConstructor::initialize(this, _Object, objectPrototype);
  JSFunctionConstructor::initialize(this, _Function, functionPrototype);
  _Array = JSArrayConstructor::initialize(this);
  _GeneratorFunction = JSGeneratorFunctionConstructor::initialize(this);
  _Iterator = JSIteratorConstructor::initialize(this);
  _Generator = JSGeneratorConstructor::initialize(this);
  _Promise = JSPromiseConstructor::initialize(this);
  _Error = JSErrorConstructor::initialize(this);
  _AggregateError = JSAggregateErrorConstructor::initialize(this);
  _RangeError = JSRangeErrorConstructor::initialize(this);
  _ReferenceError = JSReferenceErrorConstructor::initialize(this);
  _SyntaxError = JSSyntaxErrorConstructor::initialize(this);
  _TypeError = JSTypeErrorConstructor::initialize(this);
  _URIError = JSURIErrorConstructor::initialize(this);
  _InternalError = JSInternalErrorConstructor::initialize(this);
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
uint32_t JSContext::createMicroTask(common::AutoPtr<JSValue> exec) {
  static uint32_t index = 0;
  _microTasks.push_back({
      .identifier = index++,
      .exec = getRoot()->createValue(exec->getStore()),
  });
  return _microTasks.rbegin()->identifier;
}

uint32_t JSContext::createMacroTask(common::AutoPtr<JSValue> exec,
                                    int64_t timeout) {
  static uint32_t index = 0;
  _macroTasks.push_back({
      .identifier = index++,
      .exec = getRoot()->createValue(exec->getStore()),
      .timeout = timeout,
      .start = std::chrono::system_clock::now(),
  });
  return _macroTasks.rbegin()->identifier;
}
bool JSContext::nextTick() {
  using namespace std::chrono;
  if (_microTasks.empty() && _macroTasks.empty()) {
    return false;
  }
  pushScope();
  while (!_microTasks.empty()) {
    auto task = *_microTasks.begin();
    _microTasks.erase(_microTasks.begin());
    auto err = task.exec->apply(this, undefined());
    if (err->isException()) {
      fmt::print(L"Uncaught {}\n", err->toString(this)->getString().value());
    }
  }
  if (!_macroTasks.empty()) {
    auto task = *_macroTasks.begin();
    _macroTasks.erase(_macroTasks.begin());
    if (std::chrono::system_clock::now() - task.start > task.timeout * 1ms) {
      auto err = task.exec->apply(this, undefined());
      if (err->isException()) {
        fmt::print(L"Uncaught {}\n", err->toString(this)->getString().value());
      }
    } else {
      _macroTasks.push_back(task);
      std::this_thread::sleep_for(10ms);
    }
  }
  popScope();
  return true;
}

void JSContext::removeMacroTask(uint32_t id) {
  for (auto it = _macroTasks.begin(); it != _macroTasks.end(); it++) {
    if (it->identifier == id) {
      _macroTasks.erase(it);
      return;
    }
  }
}

common::AutoPtr<JSValue>
JSContext::applyGenerator(common::AutoPtr<JSValue> func,
                          common::AutoPtr<JSValue> arguments,
                          common::AutoPtr<JSValue> self) {
  auto entity = func->getEntity<JSFunctionEntity>();
  auto closure = entity->getClosure();
  auto result = constructObject(Generator());
  common::AutoPtr scope = new engine::JSScope(getRoot());
  for (auto &[name, value] : closure) {
    scope->createValue(value, name);
  }
  scope->createValue(self->getStore(), L"this");
  scope->createValue(arguments->getStore(), L"arguments");
  result->setOpaque(vm::JSCoroutineContext{
      .eval = new vm::JSEvalContext,
      .scope = scope,
      .module = entity->getModule(),
      .funcname = func->getName(),
      .pc = entity->getAddress(),
  });
  result->getStore()->appendChild(func->getStore());
  return result;
}
common::AutoPtr<JSValue>
JSContext::applyAsync(common::AutoPtr<JSValue> func,
                      common::AutoPtr<JSValue> arguments,
                      common::AutoPtr<JSValue> self) {
  auto generator = applyGenerator(func, arguments, self);
  common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
  closure[L"#generator"] = generator;
  std::function callback = [](common::AutoPtr<JSContext> ctx,
                              common::AutoPtr<JSValue> self,
                              std::vector<common::AutoPtr<JSValue>> args)
      -> common::AutoPtr<JSValue> {
    auto resolve = args[0];
    auto reject = args[1];
    auto next = [](common::AutoPtr<JSContext> ctx,
                   common::AutoPtr<JSValue> self,
                   std::vector<common::AutoPtr<JSValue>> args)
        -> common::AutoPtr<JSValue> {
      auto arg = ctx->undefined();
      if (!args.empty()) {
        arg = args[0];
      }
      auto resolve = ctx->load(L"#resolve");
      auto reject = ctx->load(L"#reject");
      auto next = ctx->load(L"#next");
      auto generator = ctx->load(L"#generator");
      auto gnext = generator->getProperty(ctx, L"next");
      auto res = gnext->apply(ctx, generator, {arg});
      if (res->isException()) {
        reject->apply(ctx, ctx->undefined(), {res});
        return ctx->undefined();
      }
      auto value = res->getProperty(ctx, L"value");
      auto done = res->getProperty(ctx, L"done");
      if (done->toBoolean(ctx)->getBoolean().value()) {
        resolve->apply(ctx, ctx->undefined(), {value});
      } else {
        auto promiseValue = ctx->Promise()
                                ->getProperty(ctx, L"resolve")
                                ->apply(ctx, ctx->Promise(), {value});
        if (promiseValue->isException()) {
          return promiseValue;
        }
        auto onerror = [](common::AutoPtr<JSContext> ctx,
                          common::AutoPtr<JSValue> self,
                          std::vector<common::AutoPtr<JSValue>> args)
            -> common::AutoPtr<JSValue> {
          auto generator = ctx->load(L"#generator");
          auto err = generator->getProperty(ctx, L"throw")
                         ->apply(ctx, generator, {args[0]});
          if (err->isException()) {
            return err;
          }
          return ctx->undefined();
        };
        common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
        closure[L"#generator"] = generator;
        promiseValue =
            promiseValue->getProperty(ctx, L"catch")
                ->apply(ctx, promiseValue,
                        {ctx->createNativeFunction(onerror, closure)});
        if (promiseValue->isException()) {
          return promiseValue;
        }
        promiseValue = promiseValue->getProperty(ctx, L"catch")
                           ->apply(ctx, promiseValue, {reject});
        if (promiseValue->isException()) {
          return promiseValue;
        }
        promiseValue = promiseValue->getProperty(ctx, L"then")
                           ->apply(ctx, promiseValue, {next});
        if (promiseValue->isException()) {
          return promiseValue;
        }
      }
      return ctx->undefined();
    };
    common::Map<std::wstring, common::AutoPtr<JSValue>> closure;
    closure[L"#resolve"] = resolve;
    closure[L"#reject"] = reject;
    closure[L"#generator"] = ctx->load(L"#generator");
    auto nextFunc = ctx->createNativeFunction(next, closure);
    nextFunc->getEntity<JSNativeFunctionEntity>()->getClosure()[L"#next"] =
        nextFunc->getStore();
    auto err = nextFunc->apply(ctx, self, {});
    if (err->isException()) {
      reject->apply(ctx, ctx->undefined(), {ctx->createError(err)});
    }
    return ctx->undefined();
  };
  auto callbackFunc = createNativeFunction(callback, closure);
  return constructObject(_Promise, {callbackFunc});
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
  return constructObject(_Array);
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
  if (constructor->getStore() == _Array->getStore()) {
    result = createValue(new JSStore(new JSArrayEntity(prototype->getStore())));
  } else if (constructor->getStore() == _Function->getStore()) {
    result = createValue(
        new JSStore(new JSFunctionEntity(prototype->getStore(), nullptr)));
  } else if (constructor->getStore() == _Promise->getStore()) {
    auto entity = new JSPromiseEntity(prototype->getStore());
    auto store = new JSStore(entity);
    result = createValue(store);
  } else if (constructor->getStore() == _Symbol->getStore()) {
    throw error::JSTypeError(L"Symbol is not a constructor");
  } else {
    result = createObject(prototype, name);
  }
  result->getStore()->appendChild(prototype->getStore());
  result->setPropertyDescriptor(this, L"constructor", constructor);
  auto err = constructor->apply(this, result, args, loc);
  if (err->isException()) {
    return err;
  }
  return result;
}

common::AutoPtr<JSValue>
JSContext::createError(common::AutoPtr<JSValue> exception,
                       const std::wstring &name) {
  auto e = exception->getEntity<JSExceptionEntity>();
  if (e->getTarget()) {
    return createValue(e->getTarget());
  }
  common::AutoPtr<JSValue> error;
  if (e->getExceptionType() == L"AggregateError") {
    error = _AggregateError;
  } else if (e->getExceptionType() == L"InternalError") {
    error = _InternalError;
  } else if (e->getExceptionType() == L"RangeError") {
    error = _RangeError;
  } else if (e->getExceptionType() == L"ReferenceError") {
    error = _ReferenceError;
  } else if (e->getExceptionType() == L"SyntaxError") {
    error = _SyntaxError;
  } else if (e->getExceptionType() == L"URIError") {
    error = _URIError;
  } else {
    error = _Error;
  }
  auto result = constructObject(error, {createString(e->getMessage())});
  auto stack = e->getStack();
  std::wstring st;
  for (auto &[fnindex, line, column, funcname] : e->getStack()) {
    auto &filename = getRuntime()->getSourceFilename(fnindex);
    if (fnindex != 0) {
      st +=
          fmt::format(L"\n at {}({}:{}:{})", funcname, filename, line, column);
    } else {
      st += fmt::format(L"\n at {} ({})", funcname, filename);
    }
  }
  result->setProperty(this, L"stack", createString(st));
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
  auto res = _scope->createValue(store, name);
  res->setPropertyDescriptor(this, L"prototype", createObject());
  return res;
}

common::AutoPtr<JSValue> JSContext::createNativeFunction(
    const std::function<JSFunction> &value,
    const common::Map<std::wstring, common::AutoPtr<JSValue>> closure,
    const std::wstring &funcname, const std::wstring &name) {
  auto prop = _Function->getProperty(this, L"prototype")->getStore();
  common::Map<std::wstring, JSStore *> clo;
  for (auto &[k, v] : closure) {
    clo[k] = (JSStore *)v->getStore();
  }
  auto res = _scope->createValue(
      new JSStore(new JSNativeFunctionEntity(prop, funcname, value, clo)),
      name);
  for (auto &[_, v] : clo) {
    res->getStore()->appendChild(v);
  }
  res->getStore()->appendChild(prop);
  res->setPropertyDescriptor(this, L"prototype", createObject());
  return res;
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
  res->setPropertyDescriptor(this, L"prototype", createObject());
  return res;
}

common::AutoPtr<JSValue>
JSContext::createFunction(const common::AutoPtr<compiler::JSModule> &module,
                          const std::wstring &name) {
  auto prop = _Function->getProperty(this, L"prototype")->getStore();
  auto res = _scope->createValue(
      new JSStore(new JSFunctionEntity(prop, module)), name);
  res->getStore()->appendChild(prop);
  res->setPropertyDescriptor(this, L"prototype", createObject());
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
  res->setPropertyDescriptor(this, L"prototype", createObject());
  return res;
}

common::AutoPtr<JSValue>
JSContext::createArrow(const common::AutoPtr<compiler::JSModule> &module,
                       const std::wstring &name) {
  auto prop = _GeneratorFunction->getProperty(this, L"prototype")->getStore();
  auto store = new JSStore(new JSFunctionEntity(prop, module));
  auto res = _scope->createValue(store, name);
  auto self = getScope()->getValue(L"this");
  if (self == nullptr) {
    res->setBind(this, undefined());
  } else {
    res->setBind(this, self);
  }
  res->getStore()->appendChild(prop);
  res->setPropertyDescriptor(this, L"prototype", createObject());
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

common::AutoPtr<JSValue> JSContext::undefined() {
  return createValue(new JSStore(new JSUndefinedEntity()));
}

common::AutoPtr<JSValue> JSContext::null() {
  return createValue(new JSStore(new JSNullEntity()));
}

common::AutoPtr<JSValue> JSContext::NaN() {
  return createValue(new JSStore(new JSNaNEntity()));
}

common::AutoPtr<JSValue> JSContext::truly() { return createBoolean(true); }

common::AutoPtr<JSValue> JSContext::falsely() { return createBoolean(false); }

common::AutoPtr<JSValue> JSContext::Symbol() { return _Symbol; }

common::AutoPtr<JSValue> JSContext::Function() { return _Function; }

common::AutoPtr<JSValue> JSContext::Object() { return _Object; }

common::AutoPtr<JSValue> JSContext::Error() { return _Error; }

common::AutoPtr<JSValue> JSContext::AggregateError() { return _AggregateError; }

common::AutoPtr<JSValue> JSContext::InternalError() { return _InternalError; }

common::AutoPtr<JSValue> JSContext::RangeError() { return _RangeError; }

common::AutoPtr<JSValue> JSContext::ReferenceError() { return _ReferenceError; }

common::AutoPtr<JSValue> JSContext::SyntaxError() { return _SyntaxError; }

common::AutoPtr<JSValue> JSContext::TypeError() { return _TypeError; }

common::AutoPtr<JSValue> JSContext::URIErrorError() { return _URIError; }

common::AutoPtr<JSValue> JSContext::Array() { return _Array; }

common::AutoPtr<JSValue> JSContext::Iterator() { return _Iterator; }

common::AutoPtr<JSValue> JSContext::ArrayIterator() { return _ArrayIterator; }

common::AutoPtr<JSValue> JSContext::GeneratorFunction() {
  return _GeneratorFunction;
}

common::AutoPtr<JSValue> JSContext::Generator() { return _Generator; }

common::AutoPtr<JSValue> JSContext::Promise() { return _Promise; }

common::AutoPtr<JSValue> JSContext::symbolValue() { return _symbolValue; }

common::AutoPtr<JSValue> JSContext::symbolPack() { return _symbolPack; }

common::AutoPtr<JSValue> JSContext::uninitialized() {
  return createValue(new JSStore(new JSEntity(JSValueType::JS_UNINITIALIZED)));
}

common::AutoPtr<JSValue> JSContext::load(const std::wstring &name) {
  auto val = _scope->getValue(name);
  if (!val) {
    throw error::JSSyntaxError(fmt::format(L"'{}' is not defined", name));
  }
  return val;
}