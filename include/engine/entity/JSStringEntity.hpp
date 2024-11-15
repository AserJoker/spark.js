#pragma once
#include "engine/entity/JSEntity.hpp"
#include <string>
namespace spark::engine {

class JSStringEntity : public JSBaseEntity<std::wstring> {
public:
  JSStringEntity(const std::wstring &value = L"");

  std::wstring &getValue();

  const std::wstring &getValue() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
  
  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine