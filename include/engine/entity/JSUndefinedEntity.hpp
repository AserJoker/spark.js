#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
struct JSUndefinedData {};
class JSUndefinedEntity : public JSBaseEntity<JSUndefinedData> {
public:
  JSUndefinedEntity();

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  
  std::wstring getTypeName(common::AutoPtr<JSContext> ctx) const override;
};
}; // namespace spark::engine