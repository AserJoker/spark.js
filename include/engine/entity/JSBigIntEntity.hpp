#pragma once
#include "common/BigInt.hpp"
#include "engine/entity/JSEntity.hpp"
#include <string>
namespace spark::engine {
class JSBigIntEntity : public JSEntity {
private:
  common::BigInt<> _value;

public:
  JSBigIntEntity(const common::BigInt<> &val);

  const common::BigInt<> &getValue() const;

  common::BigInt<> &getValue();
  
  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;

};
}; // namespace spark::engine