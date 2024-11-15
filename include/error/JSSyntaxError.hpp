#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSSyntaxError : public JSError {
public:
  JSSyntaxError(const std::wstring &message,
                const engine::JSLocation &location = {})
      : JSError(message, L"SyntaxError", location) {}
};
} // namespace spark::error