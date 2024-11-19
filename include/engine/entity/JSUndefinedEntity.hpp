#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
class JSUndefinedEntity : public JSEntity {
public:
  JSUndefinedEntity();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine