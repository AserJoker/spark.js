#pragma once
#include "common/AutoPtr.hpp"
#include "common/Map.hpp"
#include "common/Object.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/base/JSValueType.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSRuntime.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include <functional>
#include <string>

namespace spark::engine {
class JSContext : public common::Object {
public:
  struct JSFrame {
    JSFrame *parent;
    JSLocation location;
    JSFrame() {
      parent = nullptr;
      location.funcname = L"<internal>";
      location.filename = 0;
      location.line = 0;
      location.column = 0;
    }
  };

private:
  JSScope *_scope;
  common::AutoPtr<JSRuntime> _runtime;
  JSFrame *_callStack;

public:
  JSContext(const common::AutoPtr<JSRuntime> &runtime);

  ~JSContext() override;

  common::AutoPtr<JSRuntime> &getRuntime();

  JSScope *pushScope();

  void popScope(JSScope *scope);

  void setScope(JSScope *scope);

  JSScope *getScope();

  void pushCallStack(const std::wstring &funcname, const JSLocation &location);

  void popCallStack();

  JSFrame *getCallStack();

  common::Array<JSLocation> trace(const JSLocation &location);

  common::AutoPtr<JSValue> createValue(JSEntity *entity,
                                       const std::wstring &name = L"");

  common::AutoPtr<JSValue> createValue(common::AutoPtr<JSValue> value,
                                       const std::wstring &name = L"");

  common::AutoPtr<JSValue> createNumber(double value = .0f,
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createString(const std::wstring &value = L"",
                                        const std::wstring &name = L"");

  common::AutoPtr<JSValue> createBoolean(bool value = false,
                                         const std::wstring &name = L"");

  common::AutoPtr<JSValue> createInfinity(bool negative = false,
                                          const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createFunction(const std::function<JSFunction> &value,
                 const std::wstring &funcname = L"",
                 const std::wstring &name = L"");

  common::AutoPtr<JSValue>
  createFunction(const std::function<JSFunction> &value,
                 const common::Map<std::wstring, JSEntity *> closure,
                 const std::wstring &funcname = L"",
                 const std::wstring &name = L"");

  common::AutoPtr<JSValue> createException(const std::wstring &type,
                                           const std::wstring &message,
                                           const JSLocation &location = {});

  common::AutoPtr<JSValue> Undefined();

  common::AutoPtr<JSValue> Null();

  common::AutoPtr<JSValue> NaN();
};
} // namespace spark::engine