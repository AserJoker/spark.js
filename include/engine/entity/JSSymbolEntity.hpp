#pragma once
#include "common/AutoPtr.hpp"
#include "engine/entity/JSEntity.hpp"
#include <string>
namespace spark::engine {

class JSSymbolEntity : public JSEntity {
private:
  std::wstring _description;

public:
  JSSymbolEntity(const std::wstring &description);

  const std::wstring &getDescription() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  std::optional<double> toNumber(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

};
} // namespace spark::engine