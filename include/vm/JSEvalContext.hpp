#pragma once
#include "JSErrorFrame.hpp"
#include "common/AutoPtr.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"

namespace spark::vm {
struct JSEvalContext : public common::Object {
  std::vector<common::AutoPtr<engine::JSValue>> stack;
  std::vector<common::AutoPtr<engine::JSScope>> scopeChain;
  common::AutoPtr<engine::JSValue> callee;
  size_t pc;
  JSErrorFrame *errorStacks;
  JSEvalContext(size_t pc) {
    errorStacks = nullptr;
    this->pc = pc;
  };
};
} // namespace spark::vm