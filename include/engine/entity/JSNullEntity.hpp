#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
class JSNullEntity : public JSEntity {
public:
  JSNullEntity();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

};
}; // namespace spark::engine