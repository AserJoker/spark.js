#pragma once
#include "engine/base/JSLocation.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/runtime/JSStore.hpp"
#include <string>
namespace spark::engine {
struct JSExceptionData {};
class JSExceptionEntity : public JSEntity {
private:
  std::wstring _errorType;
  std::wstring _message;
  std::vector<JSLocation> _stack;
  JSStore *_target;

public:
  JSExceptionEntity(const std::wstring &type, const std::wstring &message,
                    const std::vector<JSLocation> &stack);

  JSExceptionEntity(JSStore *target);

  JSStore *getTarget();

  const std::wstring &getMessage() const;

  const std::wstring &getExceptionType() const;

  const std::vector<JSLocation> &getStack() const;

  std::wstring toString(common::AutoPtr<JSContext> ctx) const override;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine