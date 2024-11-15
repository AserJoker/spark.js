#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSInternalError : public JSError {
public:
  JSInternalError(const std::wstring &message,
                  const engine::JSLocation &location = {})
      : JSError(message, L"InternalError", location) {}
};
} // namespace spark::error