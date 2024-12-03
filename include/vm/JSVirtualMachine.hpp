#pragma once
#include "JSEvalContext.hpp"
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/base/JSAsmOperator.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/runtime/JSValue.hpp"
#include "error/JSError.hpp"
#include <string>
#define JS_OPT(name)                                                           \
  void name(common::AutoPtr<engine::JSContext> ctx,                            \
            const common::AutoPtr<compiler::JSModule> &module)
namespace spark::engine {
class JSContext;
};
namespace spark::vm {
class JSVirtualMachine : public common::Object {
private:
  common::AutoPtr<JSEvalContext> _ctx;
  size_t _pc;
  common::AutoPtr<engine::JSValue> _callee;

private:
  compiler::JSAsmOperator
  next(const common::AutoPtr<compiler::JSModule> &module);

  uint32_t argi(const common::AutoPtr<compiler::JSModule> &module);

  double argf(const common::AutoPtr<compiler::JSModule> &module);

  const std::wstring &args(const common::AutoPtr<compiler::JSModule> &module);

  void handleError(common::AutoPtr<engine::JSContext> ctx,
                   const common::AutoPtr<compiler::JSModule> &module,
                   const error::JSError &e);

  void handleError(common::AutoPtr<engine::JSContext> ctx,
                   const common::AutoPtr<compiler::JSModule> &module,
                   common::AutoPtr<engine::JSValue> e);

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
  JS_OPT(setFuncSource);
  JS_OPT(setClosure);
  JS_OPT(setField);
  JS_OPT(getField);
  JS_OPT(setAccessor);
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
  JS_OPT(throw_);
  JS_OPT(new_);
  JS_OPT(yield);
  JS_OPT(yieldDelegate);
  JS_OPT(await);
  JS_OPT(nullishCoalescing);
  JS_OPT(pushScope);
  JS_OPT(popScope);
  JS_OPT(call);
  JS_OPT(memberCall);
  JS_OPT(tryStart);
  JS_OPT(tryEnd);
  JS_OPT(defer);
  JS_OPT(jmp);
  JS_OPT(add);

public:
  JSVirtualMachine();

  common::AutoPtr<engine::JSValue>
  eval(common::AutoPtr<engine::JSContext> ctx,
       const common::AutoPtr<compiler::JSModule> &module, size_t offset = 0);

  common::AutoPtr<engine::JSValue>
  apply(common::AutoPtr<engine::JSContext> ctx,
        common::AutoPtr<engine::JSValue> func,
        common::AutoPtr<engine::JSValue> self,
        std::vector<common::AutoPtr<engine::JSValue>> args);
};
}; // namespace spark::vm