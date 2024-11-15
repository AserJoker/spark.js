#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSReferenceError : public JSError {
public:
  JSReferenceError(const std::wstring &message,
                   const engine::JSLocation &location = {})
      : JSError(message, L"ReferenceError", location) {}
};
} // namespace spark::error