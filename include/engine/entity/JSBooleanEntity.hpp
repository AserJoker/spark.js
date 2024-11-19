#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
class JSBooleanEntity : public JSEntity {
private:
  bool _value;

public:
  JSBooleanEntity(bool value = false);

  bool &getValue();

  bool getValue() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

};
} // namespace spark::engine