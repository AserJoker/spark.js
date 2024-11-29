#pragma once
#include "JSErrorFrame.hpp"
#include "engine/runtime/JSScope.hpp"
#include "engine/runtime/JSValue.hpp"

namespace spark::vm {
struct JSEvalContext {
  std::vector<size_t> stackFrames;
  std::vector<engine::JSScope *> scopeChain;
  size_t pc;
  JSErrorFrame *errorStacks;
  JSEvalContext *parent;
  JSEvalContext(JSEvalContext *parent, size_t pc) {
    errorStacks = nullptr;
    this->parent = parent;
    this->pc = pc;
  };
};
} // namespace spark::vm