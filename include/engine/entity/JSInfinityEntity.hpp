#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
class JSInfinityEntity : public JSEntity {
private:
  bool _negative;

public:
  JSInfinityEntity(bool negative = false);

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;

  bool isNegative() const;

  void negative();
};
} // namespace spark::engine