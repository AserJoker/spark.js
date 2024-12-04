#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
namespace spark::vm {
struct JSErrorFrame : public common::Object {
  uint32_t defer;
  uint32_t handle;
  common::AutoPtr<JSErrorFrame> parent;
  JSErrorFrame(const common::AutoPtr<JSErrorFrame> &parent)
      : defer(0), handle(0) {
    this->parent = parent;
  }
};
} // namespace spark::vm