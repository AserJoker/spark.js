#pragma once
#include "JSErrorFrame.hpp"
#include "common/AutoPtr.hpp"
#include "engine/runtime/JSValue.hpp"

namespace spark::vm {
struct JSEvalContext : public common::Object {
  std::vector<common::AutoPtr<engine::JSValue>> stack;
  JSErrorFrame *errorStacks;
  JSEvalContext() { errorStacks = nullptr; };
};
} // namespace spark::vm