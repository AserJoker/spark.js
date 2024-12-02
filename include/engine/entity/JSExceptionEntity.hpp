#pragma once
#include "engine/base/JSLocation.hpp"
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <string>
namespace spark::engine {
struct JSExceptionData {};
class JSExceptionEntity : public JSObjectEntity {
private:
  std::wstring _errorType;
  std::wstring _message;
  std::vector<JSLocation> _stack;
  JSEntity *_target;

public:
  JSExceptionEntity(JSEntity *prototype, const std::wstring &type,
                    const std::wstring &message,
                    const std::vector<JSLocation> &stack);

  JSExceptionEntity(JSEntity *prototype, JSEntity *target);

  JSEntity *getTarget();

  const std::wstring &getMessage() const;

  const std::wstring &getExceptionType() const;

  const std::vector<JSLocation> &getStack() const;

  bool toBoolean(common::AutoPtr<JSContext> ctx) const override;
};
} // namespace spark::engine