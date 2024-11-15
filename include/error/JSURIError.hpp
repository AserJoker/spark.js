#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSURIError : public JSError {
public:
  JSURIError(const std::wstring &message,
             const engine::JSLocation &location = {})
      : JSError(message, L"URIError", location) {}
};
} // namespace spark::error