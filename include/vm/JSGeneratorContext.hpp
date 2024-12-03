#pragma once
#include "common/AutoPtr.hpp"
#include "common/Object.hpp"
#include "engine/runtime/JSValue.hpp"
#include "vm/JSEvalContext.hpp"
namespace spark::vm {
struct JSGeneratorContext : public common::Object {
  common::AutoPtr<JSEvalContext> eval;
  common::AutoPtr<engine::JSValue> func;
};
} // namespace spark::vm