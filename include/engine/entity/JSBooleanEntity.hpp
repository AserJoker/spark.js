#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSBooleanData {
  bool value;
};
class JSBooleanEntity : public JSBaseEntity<JSBooleanData> {
public:
  JSBooleanEntity(bool value = false);

  bool &getValue();

  bool getValue() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine