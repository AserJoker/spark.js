#pragma once
#include "JSEvalContext.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"
#include <string>
namespace spark::vm {
struct JSCoroutineContext {
  common::AutoPtr<JSEvalContext> eval;
  common::AutoPtr<engine::JSScope> scope;
  common::AutoPtr<compiler::JSModule> module;
  common::AutoPtr<engine::JSValue> value;
  bool done;
  std::wstring funcname;
  size_t pc;
  ~JSCoroutineContext() {
    while (scope != nullptr) {
      scope = scope->getParent();
    }
  }
};
}; // namespace spark::vm