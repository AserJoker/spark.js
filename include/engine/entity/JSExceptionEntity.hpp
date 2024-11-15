#pragma once
#include "common/Array.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/entity/JSEntity.hpp"
#include <string>
namespace spark::engine {
struct JSExceptionData {
  std::wstring type;
  std::wstring message;
  common::Array<JSLocation> stack;
};
class JSExceptionEntity : public JSBaseEntity<JSExceptionData> {
public:
  JSExceptionEntity(const std::wstring &type, const std::wstring &message,
                    const common::Array<JSLocation> &stack);

  const std::wstring &getMessage() const;

  const std::wstring &getExceptionType() const;

  const common::Array<JSLocation> &getStack() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine