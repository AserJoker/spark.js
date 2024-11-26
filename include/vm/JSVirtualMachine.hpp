#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "compiler/base/JSAsmOperator.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>
#include <vector>
namespace spark::engine {
class JSContext;
};
namespace spark::vm {
class JSVirtualMachine : public common::Object {
private:
  std::vector<common::AutoPtr<engine::JSValue>> _stack;
  std::vector<size_t> _stackTops;
  std::vector<engine::JSScope *> _scopeChain;
  size_t _pc;

private:
  compiler::JSAsmOperator
  next(const common::AutoPtr<compiler::JSModule> &module);

  uint32_t argi(const common::AutoPtr<compiler::JSModule> &module);
  double argf(const common::AutoPtr<compiler::JSModule> &module);
  const std::wstring &args(const common::AutoPtr<compiler::JSModule> &module);

private:
  void pushNull(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void pushUndefined(common::AutoPtr<engine::JSContext> ctx,
                     const common::AutoPtr<compiler::JSModule> &module);
  void pushTrue(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void pushFalse(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void pushUninitialized(common::AutoPtr<engine::JSContext> ctx,
                         const common::AutoPtr<compiler::JSModule> &module);
  void push(common::AutoPtr<engine::JSContext> ctx,
            const common::AutoPtr<compiler::JSModule> &module);
  void pushObject(common::AutoPtr<engine::JSContext> ctx,
                  const common::AutoPtr<compiler::JSModule> &module);
  void pushArray(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void pushFunction(common::AutoPtr<engine::JSContext> ctx,
                    const common::AutoPtr<compiler::JSModule> &module);
  void pushGenerator(common::AutoPtr<engine::JSContext> ctx,
                     const common::AutoPtr<compiler::JSModule> &module);
  void pushArrow(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void pushThis(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void pushSuper(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void pushArgument(common::AutoPtr<engine::JSContext> ctx,
                    const common::AutoPtr<compiler::JSModule> &module);
  void pushBigint(common::AutoPtr<engine::JSContext> ctx,
                  const common::AutoPtr<compiler::JSModule> &module);
  void pushRegex(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void setAddress(common::AutoPtr<engine::JSContext> ctx,
                  const common::AutoPtr<compiler::JSModule> &module);
  void setAsync(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void setFuncName(common::AutoPtr<engine::JSContext> ctx,
                   const common::AutoPtr<compiler::JSModule> &module);
  void setFuncLen(common::AutoPtr<engine::JSContext> ctx,
                  const common::AutoPtr<compiler::JSModule> &module);
  void setClosure(common::AutoPtr<engine::JSContext> ctx,
                  const common::AutoPtr<compiler::JSModule> &module);
  void setField(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void getField(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void setAccessor(common::AutoPtr<engine::JSContext> ctx,
                   const common::AutoPtr<compiler::JSModule> &module);
  void getAccessor(common::AutoPtr<engine::JSContext> ctx,
                   const common::AutoPtr<compiler::JSModule> &module);
  void setMethod(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void getMethod(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void setIndex(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void getIndex(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void setRegexHasIndices(common::AutoPtr<engine::JSContext> ctx,
                          const common::AutoPtr<compiler::JSModule> &module);
  void setRegexGlobal(common::AutoPtr<engine::JSContext> ctx,
                      const common::AutoPtr<compiler::JSModule> &module);
  void setRegexIgnoreCases(common::AutoPtr<engine::JSContext> ctx,
                           const common::AutoPtr<compiler::JSModule> &module);
  void setRegexMultiline(common::AutoPtr<engine::JSContext> ctx,
                         const common::AutoPtr<compiler::JSModule> &module);
  void setRegexDotAll(common::AutoPtr<engine::JSContext> ctx,
                      const common::AutoPtr<compiler::JSModule> &module);
  void setRegexSticky(common::AutoPtr<engine::JSContext> ctx,
                      const common::AutoPtr<compiler::JSModule> &module);
  void pop(common::AutoPtr<engine::JSContext> ctx,
           const common::AutoPtr<compiler::JSModule> &module);
  void storeConst(common::AutoPtr<engine::JSContext> ctx,
                  const common::AutoPtr<compiler::JSModule> &module);
  void store(common::AutoPtr<engine::JSContext> ctx,
             const common::AutoPtr<compiler::JSModule> &module);
  void load(common::AutoPtr<engine::JSContext> ctx,
            const common::AutoPtr<compiler::JSModule> &module);
  void loadConst(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void ret(common::AutoPtr<engine::JSContext> ctx,
           const common::AutoPtr<compiler::JSModule> &module);
  void yield(common::AutoPtr<engine::JSContext> ctx,
             const common::AutoPtr<compiler::JSModule> &module);
  void await(common::AutoPtr<engine::JSContext> ctx,
             const common::AutoPtr<compiler::JSModule> &module);
  void nullishCoalescing(common::AutoPtr<engine::JSContext> ctx,
                         const common::AutoPtr<compiler::JSModule> &module);
  void pushScope(common::AutoPtr<engine::JSContext> ctx,
                 const common::AutoPtr<compiler::JSModule> &module);
  void popScope(common::AutoPtr<engine::JSContext> ctx,
                const common::AutoPtr<compiler::JSModule> &module);
  void call(common::AutoPtr<engine::JSContext> ctx,
            const common::AutoPtr<compiler::JSModule> &module);
  void add(common::AutoPtr<engine::JSContext> ctx,
           const common::AutoPtr<compiler::JSModule> &module);

public:
  JSVirtualMachine();

  common::AutoPtr<engine::JSValue>
  run(common::AutoPtr<engine::JSContext> ctx,
      const common::AutoPtr<compiler::JSModule> &module, size_t offset = 0);
};
}; // namespace spark::vm