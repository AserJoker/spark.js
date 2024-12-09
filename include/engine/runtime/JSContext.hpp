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
#include "engine/runtime/JSValue.hpp"
#include <functional>
#include <string>
#include <unordered_map>

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

private:
  common::AutoPtr<JSValue> _Object;
  common::AutoPtr<JSValue> _Function;
  common::AutoPtr<JSValue> _GeneratorFunction;
  common::AutoPtr<JSValue> _Generator;
  common::AutoPtr<JSValue> _Iterator;
  common::AutoPtr<JSValue> _ArrayIterator;
  common::AutoPtr<JSValue> _Array;
  common::AutoPtr<JSValue> _Error;
  common::AutoPtr<JSValue> _Symbol;
  common::AutoPtr<JSValue> _Number;
  common::AutoPtr<JSValue> _String;
  common::AutoPtr<JSValue> _Boolean;
  common::AutoPtr<JSValue> _BigInt;
  common::AutoPtr<JSValue> _RegExp;

  // internal
  std::unordered_map<std::wstring, JSStore *> _symbols;

  common::AutoPtr<JSValue> _symbolValue;
  common::AutoPtr<JSValue> _symbolPack;

private:
  void initialize();

private:
  common::AutoPtr<JSScope> _root;
  common::AutoPtr<JSScope> _scope;
  common::AutoPtr<JSRuntime> _runtime;
  JSCallFrame *_callStack;

public:
  JSContext(const common::AutoPtr<JSRuntime> &runtime);

  ~JSContext() override;

  common::AutoPtr<JSRuntime> &getRuntime();

  common::AutoPtr<JSValue> eval(const std::wstring &source,
                                const std::wstring &filename);

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
                  const std::vector<common::AutoPtr<JSValue>> &args,
                  const JSLocation &loc, const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createNativeFunction(const std::function<JSFunction> &value,
                       const std::wstring &funcname = L"",
                       const std::wstring &name = L"");

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

  common::AutoPtr<JSValue> Error();

  common::AutoPtr<JSValue> Array();

  common::AutoPtr<JSValue> Iterator();

  common::AutoPtr<JSValue> ArrayIterator();

  common::AutoPtr<JSValue> GeneratorFunction();

  common::AutoPtr<JSValue> Generator();

  common::AutoPtr<JSValue> uninitialized();

  common::AutoPtr<JSValue> symbolValue();

  common::AutoPtr<JSValue> symbolPack();

  common::AutoPtr<JSValue> load(const std::wstring &name);
};
} // namespace spark::engine