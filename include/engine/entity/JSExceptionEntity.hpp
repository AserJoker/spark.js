#pragma once
#include "common/Array.hpp"
#include "engine/base/JSLocation.hpp"
#include "engine/entity/JSEntity.hpp"
#include <string>
namespace spark::engine {
struct JSExceptionData {};
class JSExceptionEntity : public JSEntity {
private:
  std::wstring _type;
  std::wstring _message;
  std::vector<JSLocation> _stack;

public:
  JSExceptionEntity(const std::wstring &type, const std::wstring &message,
                    const std::vector<JSLocation> &stack);

  const std::wstring &getMessage() const;

  const std::wstring &getExceptionType() const;

  const std::vector<JSLocation> &getStack() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine