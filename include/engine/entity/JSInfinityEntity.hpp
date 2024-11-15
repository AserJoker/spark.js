#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSInfinityData {};
class JSInfinityEntity : public JSBaseEntity<JSInfinityData> {
public:
  JSInfinityEntity();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine