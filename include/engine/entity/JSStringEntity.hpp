#pragma once
#include "engine/entity/JSEntity.hpp"
#include <string>
namespace spark::engine {

class JSStringEntity : public JSEntity {
private:
  std::wstring _value;

public:
  JSStringEntity(const std::wstring &value = L"");

  std::wstring &getValue();

  const std::wstring &getValue() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;

};
}; // namespace spark::engine