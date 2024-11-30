#pragma once
#include "JSErrorFrame.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"

namespace spark::vm {
struct JSEvalContext : public common::Object {
  std::vector<common::AutoPtr<engine::JSValue>> stack;
  std::vector<engine::JSScope *> scopeChain;
  size_t pc;
  JSErrorFrame *errorStacks;
  JSEvalContext(size_t pc) {
    errorStacks = nullptr;
    this->pc = pc;
  };
};
} // namespace spark::vm