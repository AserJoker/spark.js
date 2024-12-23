#pragma once
#include "JSStore.hpp"
#include "common/AutoPtr.hpp"
#include "common/BigInt.hpp"
#include "common/Map.hpp"
#include "common/Object.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSStore.hpp"
#include "engine/runtime/JSValue.hpp"
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace spark::engine {
class JSContext : public common::Object {
public:
  struct JSCallFrame {
    JSCallFrame *parent;
    JSLocation location;
    JSCallFrame() {
      parent = nullptr;
      location.funcname = L"<internal>";
      location.filename = 0;
      location.line = 0;
      location.column = 0;
    }
  };

  struct Task {
    uint32_t identifier;
    common::AutoPtr<JSValue> exec;
    int64_t timeout;
    std::chrono::system_clock::time_point start;
  };

  enum class EvalType { MODULE, BINARY, FUNCTION, EXPRESSION };

private:
  common::AutoPtr<JSValue> _Object;
  common::AutoPtr<JSValue> _Function;
  common::AutoPtr<JSValue> _GeneratorFunction;
  common::AutoPtr<JSValue> _Generator;
  common::AutoPtr<JSValue> _Iterator;
  common::AutoPtr<JSValue> _ArrayIterator;
  common::AutoPtr<JSValue> _Array;
  common::AutoPtr<JSValue> _Symbol;
  common::AutoPtr<JSValue> _Number;
  common::AutoPtr<JSValue> _String;
  common::AutoPtr<JSValue> _Boolean;
  common::AutoPtr<JSValue> _BigInt;
  common::AutoPtr<JSValue> _RegExp;
  common::AutoPtr<JSValue> _Promise;
  common::AutoPtr<JSValue> _Error;
  common::AutoPtr<JSValue> _AggregateError;
  common::AutoPtr<JSValue> _InternalError;
  common::AutoPtr<JSValue> _RangeError;
  common::AutoPtr<JSValue> _ReferenceError;
  common::AutoPtr<JSValue> _TypeError;
  common::AutoPtr<JSValue> _SyntaxError;
  common::AutoPtr<JSValue> _URIError;

  // internal
  std::unordered_map<std::wstring, JSStore *> _symbols;

  common::AutoPtr<JSValue> _symbolValue;
  common::AutoPtr<JSValue> _symbolPack;

private:
  common::AutoPtr<JSScope> _root;
  common::AutoPtr<JSScope> _scope;
  common::AutoPtr<JSRuntime> _runtime;

  std::vector<Task> _microTasks;
  std::vector<Task> _macroTasks;

  std::unordered_map<std::wstring, common::AutoPtr<JSValue>> _modules;

  std::pair<std::wstring, common::AutoPtr<JSValue>> _currentModule;

  JSCallFrame *_callStack;

private:
  void initialize();

public:
  JSContext(const common::AutoPtr<JSRuntime> &runtime);

  ~JSContext() override;

  common::AutoPtr<JSRuntime> &getRuntime();

  const std::pair<std::wstring, common::AutoPtr<JSValue>> &
  getCurrentModule() const;

  std::pair<std::wstring, common::AutoPtr<JSValue>> &getCurrentModule();

  common::AutoPtr<JSValue>
  eval(const std::wstring &source, const std::wstring &filename,
       const EvalType &type = JSContext::EvalType::EXPRESSION);

  common::AutoPtr<JSValue>
  eval(const std::wstring &filename,
       const EvalType &type = JSContext::EvalType::EXPRESSION);

  common::AutoPtr<compiler::JSModule> compile(const std::wstring &source,
                                              const std::wstring &filename);

  void pushScope();

  void popScope();

  common::AutoPtr<JSScope> setScope(common::AutoPtr<JSScope> scope);

  common::AutoPtr<JSScope> getScope();

  common::AutoPtr<JSScope> getRoot();

  void pushCallStack(const JSLocation &location);

  void popCallStack();

  JSCallFrame *getCallStack();

  std::vector<JSLocation> trace(const JSLocation &location);

  uint32_t createMicroTask(common::AutoPtr<JSValue> exec);

  uint32_t createMacroTask(common::AutoPtr<JSValue> exec, int64_t timeout = 0);

  common::AutoPtr<JSValue> nextTick();

  bool isTaskComplete() const;

  void removeMacroTask(uint32_t id);

  common::AutoPtr<JSValue> applyGenerator(common::AutoPtr<JSValue> func,
                                          common::AutoPtr<JSValue> arguments,
                                          common::AutoPtr<JSValue> self);

  common::AutoPtr<JSValue> applyAsync(common::AutoPtr<JSValue> func,
                                      common::AutoPtr<JSValue> arguments,
                                      common::AutoPtr<JSValue> self);

  void setModule(const std::wstring &name, common::AutoPtr<JSValue> module);

  common::AutoPtr<JSValue> getModule(const std::wstring &name);

  common::AutoPtr<JSValue> createValue(JSStore *entity,
                                       const std::wstring &name = L"");

  common::AutoPtr<JSValue> createValue(common::AutoPtr<JSValue> value,
                                       const std::wstring &name = L"");

  common::AutoPtr<JSValue> createNumber(double value = .0f,
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createString(const std::wstring &value = L"",
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createSymbol(const std::wstring &value = L"",
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createBoolean(bool value = false,
                                         const std::wstring &name = L"");

  common::AutoPtr<JSValue> createBigInt(const common::BigInt<> &value = {},
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createInfinity(bool negative = false,
                                          const std::wstring &name = L"");

  common::AutoPtr<JSValue> createObject(common::AutoPtr<JSValue> prototype,
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createObject(const std::wstring &name = L"");

  common::AutoPtr<JSValue> createArray(const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  constructObject(common::AutoPtr<JSValue> constructor,
                  const std::vector<common::AutoPtr<JSValue>> &args = {},
                  const JSLocation &loc = {}, const std::wstring &name = L"");

  common::AutoPtr<JSValue> createError(common::AutoPtr<JSValue> exception,
                                       const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createNativeFunction(const std::function<JSFunction> &value,
                       const std::wstring &funcname = L"",
                       const std::wstring &name = L"");

  common::AutoPtr<JSValue> createNativeFunction(
      const std::function<JSFunction> &value,
      const common::Map<std::wstring, common::AutoPtr<JSValue>> closure,
      const std::wstring &funcname = L"", const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createNativeFunction(const std::function<JSFunction> &value,
                       const common::Map<std::wstring, JSStore *> closure,
                       const std::wstring &funcname = L"",
                       const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createFunction(const common::AutoPtr<compiler::JSModule> &module,
                 const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createGenerator(const common::AutoPtr<compiler::JSModule> &module,
                  const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createArrow(const common::AutoPtr<compiler::JSModule> &module,
              const std::wstring &name = L"");

  common::AutoPtr<JSValue> createException(const std::wstring &type,
                                           const std::wstring &message,
                                           const JSLocation &location = {});

  common::AutoPtr<JSValue> createException(common::AutoPtr<JSValue> target);

  common::AutoPtr<JSValue> undefined();

  common::AutoPtr<JSValue> null();

  common::AutoPtr<JSValue> NaN();

  common::AutoPtr<JSValue> truly();

  common::AutoPtr<JSValue> falsely();

  common::AutoPtr<JSValue> Symbol();

  common::AutoPtr<JSValue> Function();

  common::AutoPtr<JSValue> Object();

  common::AutoPtr<JSValue> Array();

  common::AutoPtr<JSValue> Iterator();

  common::AutoPtr<JSValue> ArrayIterator();

  common::AutoPtr<JSValue> GeneratorFunction();

  common::AutoPtr<JSValue> Generator();

  common::AutoPtr<JSValue> Promise();

  common::AutoPtr<JSValue> Error();

  common::AutoPtr<JSValue> AggregateError();

  common::AutoPtr<JSValue> InternalError();

  common::AutoPtr<JSValue> RangeError();

  common::AutoPtr<JSValue> ReferenceError();

  common::AutoPtr<JSValue> SyntaxError();

  common::AutoPtr<JSValue> TypeError();

  common::AutoPtr<JSValue> URIErrorError();

  common::AutoPtr<JSValue> uninitialized();

  common::AutoPtr<JSValue> symbolValue();

  common::AutoPtr<JSValue> symbolPack();

  common::AutoPtr<JSValue> load(const std::wstring &name);
};
} // namespace spark::engine