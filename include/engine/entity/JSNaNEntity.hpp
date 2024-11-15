#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSNaNData {};
class JSNaNEntity : public JSBaseEntity<JSNaNData> {
public:
  JSNaNEntity();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;
  
  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine