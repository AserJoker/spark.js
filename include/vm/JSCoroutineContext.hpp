#pragma once
#include "JSEvalContext.hpp"
#include "common/AutoPtr.hpp"
#include "compiler/base/JSModule.hpp"
#include "engine/runtime/JSScope.hpp"
#include <string>
namespace spark::vm {
struct JSCoroutineContext {
  common::AutoPtr<JSEvalContext> eval;
  common::AutoPtr<engine::JSScope> scope;
  common::AutoPtr<compiler::JSModule> module;
  std::wstring funcname;
  size_t pc;
};
}; // namespace spark::vm