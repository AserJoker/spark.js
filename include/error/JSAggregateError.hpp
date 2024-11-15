#pragma once
#include "JSError.hpp"
#include <string>
namespace spark::error {
class JSAggregateError : public JSError {
public:
  JSAggregateError(const std::wstring &message,
                   const engine::JSLocation &location = {})
      : JSError(message, L"AggregateError", location) {}
};
} // namespace spark::error