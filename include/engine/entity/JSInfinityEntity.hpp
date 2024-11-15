#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSInfinityData {
  bool negative;
};
class JSInfinityEntity : public JSBaseEntity<JSInfinityData> {
public:
  JSInfinityEntity(bool negative = false);

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;

  bool isNegative() const;

  void negative();
};
} // namespace spark::engine