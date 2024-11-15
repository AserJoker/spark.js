#pragma once
#include "engine/base/JSLocation.hpp"
#include <codecvt>
#include <fmt/format.h>
#include <locale>
#include <stdexcept>
#include <string>
namespace spark::error {
class JSError : public std::runtime_error {
private:
  std::wstring _type;

  std::wstring _message;

  engine::JSLocation _location;

  std::string format(const std::wstring &type, const std::wstring &message) {
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return fmt::format("{} {}", converter.to_bytes(type),
                       converter.to_bytes(message));
  }

public:
  JSError(const std::wstring &message, const std::wstring &type = L"Error",
          const engine::JSLocation &location = {})
      : std::runtime_error(format(type, message)), _type(type),
        _message(message), _location(location){};

  const std::wstring &getType() { return _type; }

  const std::wstring &getMessage() { return _message; }

  const engine::JSLocation &getLocation() { return _location; }
};
} // namespace spark::error