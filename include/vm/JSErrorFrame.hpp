#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/runtime/JSScope.hpp"
namespace spark::vm {
struct JSErrorFrame : public common::Object {
  uint32_t defer;
  uint32_t handle;
  common::AutoPtr<engine::JSScope> scope;
  common::AutoPtr<JSErrorFrame> parent;
  JSErrorFrame(const common::AutoPtr<JSErrorFrame> &parent,
               const common::AutoPtr<engine::JSScope> &scope)
      : defer(0), handle(0), scope(scope) {
    this->parent = parent;
  }
};
} // namespace spark::vm