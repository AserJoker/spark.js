#pragma once
#include "JSAsmOperator.hpp"
#include "JSEvalContext.hpp"
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/runtime/JSValue.hpp"
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

private:
  vm::JSAsmOperator next(const common::AutoPtr<compiler::JSModule> &module);

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
  JS_OPT(pushAsync);
  JS_OPT(pushAsyncArrow);
  JS_OPT(pushAsyncGenerator);
  JS_OPT(pushThis);
  JS_OPT(pushSuper);
  JS_OPT(pushBigint);
  JS_OPT(pushRegex);
  JS_OPT(pushValue);
  JS_OPT(setAddress);
  JS_OPT(setFuncName);
  JS_OPT(setFuncLen);
  JS_OPT(setFuncSource);
  JS_OPT(setClosure);
  JS_OPT(setField);
  JS_OPT(getField);
  JS_OPT(getKeys);
  JS_OPT(setAccessor);
  JS_OPT(merge);
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
  JS_OPT(hlt);
  JS_OPT(throw_);
  JS_OPT(new_);
  JS_OPT(yield);
  JS_OPT(yieldDelegate);
  JS_OPT(next);
  JS_OPT(awaitNext);
  JS_OPT(restArray);
  JS_OPT(restObject);
  JS_OPT(await);
  JS_OPT(void_);
  JS_OPT(delete_);
  JS_OPT(typeof_);
  JS_OPT(pushScope);
  JS_OPT(popScope);
  JS_OPT(call);
  JS_OPT(memberCall);
  JS_OPT(optionalCall);
  JS_OPT(memberOptionalCall);
  JS_OPT(tryStart);
  JS_OPT(tryEnd);
  JS_OPT(defer);
  JS_OPT(deferEnd);
  JS_OPT(jmp);
  JS_OPT(jfalse);
  JS_OPT(jtrue);
  JS_OPT(jnotNull);
  JS_OPT(jnull);
  JS_OPT(pow);
  JS_OPT(mul);
  JS_OPT(div);
  JS_OPT(mod);
  JS_OPT(add);
  JS_OPT(sub);
  JS_OPT(inc);
  JS_OPT(dec);
  JS_OPT(plus);
  JS_OPT(netation);
  JS_OPT(not_);
  JS_OPT(logicalNot);
  JS_OPT(ushr);
  JS_OPT(shr);
  JS_OPT(shl);
  JS_OPT(le);
  JS_OPT(ge);
  JS_OPT(gt);
  JS_OPT(lt);
  JS_OPT(seq);
  JS_OPT(sne);
  JS_OPT(eq);
  JS_OPT(ne);
  JS_OPT(and_);
  JS_OPT(or_);
  JS_OPT(xor_);
  JS_OPT(instanceof);
  JS_OPT(import);
  JS_OPT(importModule);
  JS_OPT(export_);

public:
  JSVirtualMachine();

  common::AutoPtr<JSEvalContext> getContext();

  void setContext(common::AutoPtr<JSEvalContext> ctx);

  void run(common::AutoPtr<engine::JSContext> ctx,
           const common::AutoPtr<compiler::JSModule> &module, size_t offset);

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