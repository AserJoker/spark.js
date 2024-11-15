#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSNullData {};
class JSNullEntity : public JSBaseEntity<JSNullData> {
public:
  JSNullEntity();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine