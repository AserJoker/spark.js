#pragma once
#include "JSErrorFrame.hpp"
#include "common/AutoPtr.hpp"
#include "engine/runtime/JSValue.hpp"
#include <vector>

namespace spark::vm {
struct JSEvalContext : public common::Object {
  std::vector<common::AutoPtr<engine::JSValue>> stack;
  std::vector<size_t> stackTops;
  std::vector<size_t> deferStack;
  common::AutoPtr<JSErrorFrame> errorStacks;
  JSEvalContext(){};
};
} // namespace spark::vm