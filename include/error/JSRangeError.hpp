#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSRangeError : public JSError {
public:
  JSRangeError(const std::wstring &message,
               const engine::JSLocation &location = {})
      : JSError(message, L"RangeError", location) {}
};
} // namespace spark::error