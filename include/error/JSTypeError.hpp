#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSTypeError : public JSError {
public:
  JSTypeError(const std::wstring &message,
              const engine::JSLocation &location = {})
      : JSError(message, L"TypeError", location) {}
};
} // namespace spark::error