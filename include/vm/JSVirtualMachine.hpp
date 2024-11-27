#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/base/JSAsmOperator.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>
#include <vector>
#define JS_OPT(name)                                                           \
  void name(common::AutoPtr<engine::JSContext> ctx,                            \
            const common::AutoPtr<compiler::JSModule> &module)
namespace spark::engine {
class JSContext;
};
namespace spark::vm {
class JSVirtualMachine : public common::Object {
private:
  struct ErrFrame {
    uint32_t defer;
    uint32_t handle;
  };

private:
  std::vector<common::AutoPtr<engine::JSValue>> _stack;
  std::vector<size_t> _stackTops;
  std::vector<engine::JSScope *> _scopeChain;
  std::vector<ErrFrame> _tryStacks;
  size_t _pc;

private:
  compiler::JSAsmOperator
  next(const common::AutoPtr<compiler::JSModule> &module);

  uint32_t argi(const common::AutoPtr<compiler::JSModule> &module);
  double argf(const common::AutoPtr<compiler::JSModule> &module);
  const std::wstring &args(const common::AutoPtr<compiler::JSModule> &module);

private:
  JS_OPT(pushNull);
  JS_OPT(pushUndefined);
  JS_OPT(pushTrue);
  JS_OPT(pushFalse);
  JS_OPT(pushUninitialized);
  JS_OPT(push);
  JS_OPT(pushObject);
  JS_OPT(pushArray);
  JS_OPT(pushFunction);
  JS_OPT(pushGenerator);
  JS_OPT(pushArrow);
  JS_OPT(pushThis);
  JS_OPT(pushSuper);
  JS_OPT(pushArgument);
  JS_OPT(pushBigint);
  JS_OPT(pushRegex);
  JS_OPT(setAddress);
  JS_OPT(setAsync);
  JS_OPT(setFuncName);
  JS_OPT(setFuncLen);
  JS_OPT(setClosure);
  JS_OPT(setField);
  JS_OPT(getField);
  JS_OPT(setAccessor);
  JS_OPT(getAccessor);
  JS_OPT(setMethod);
  JS_OPT(getMethod);
  JS_OPT(setIndex);
  JS_OPT(getIndex);
  JS_OPT(setRegexHasIndices);
  JS_OPT(setRegexGlobal);
  JS_OPT(setRegexIgnoreCases);
  JS_OPT(setRegexMultiline);
  JS_OPT(setRegexDotAll);
  JS_OPT(setRegexSticky);
  JS_OPT(pop);
  JS_OPT(storeConst);
  JS_OPT(store);
  JS_OPT(load);
  JS_OPT(loadConst);
  JS_OPT(ret);
  JS_OPT(yield);
  JS_OPT(await);
  JS_OPT(nullishCoalescing);
  JS_OPT(pushScope);
  JS_OPT(popScope);
  JS_OPT(call);
  JS_OPT(tryStart);
  JS_OPT(tryEnd);
  JS_OPT(defer);
  JS_OPT(back);
  JS_OPT(jmp);
  JS_OPT(add);

public:
  JSVirtualMachine();

  common::AutoPtr<engine::JSValue>
  eval(common::AutoPtr<engine::JSContext> ctx,
       const common::AutoPtr<compiler::JSModule> &module, size_t offset = 0);
};
}; // namespace spark::vm