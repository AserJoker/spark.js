#pragma once
#include "engine/runtime/JSScope.hpp"
namespace spark::vm {
struct JSErrorFrame {
  common::AutoPtr<engine::JSScope> scope;
  uint32_t defer;
  uint32_t handle;
  JSErrorFrame *parent;
};
} // namespace spark::vm