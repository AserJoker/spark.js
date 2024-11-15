#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {

class JSNumberEntity : public JSBaseEntity<double> {
public:
  JSNumberEntity(double value = .0f);

  double &getValue();

  const double getValue() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
  
  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine