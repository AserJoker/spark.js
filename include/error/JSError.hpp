#pragma once
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 1
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#include "engine/base/JSLocation.hpp"
#include <codecvt>
#include <fmt/format.h>
#include <fmt/xchar.h>
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
    if (type.empty()) {
      return converter.to_bytes(message);
    }
    std::wstring msg = fmt::format(L"{}: {}", type, message);
    return converter.to_bytes(msg);
  }

public:
  JSError(const std::wstring &message, const std::wstring &type = L"Error",
          const engine::JSLocation &location = {})
      : std::runtime_error(format(type, message)), _type(type),
        _message(message), _location(location){};

  const std::wstring &getType() const { return _type; }

  const std::wstring &getMessage() const { return _message; }

  const engine::JSLocation &getLocation() const { return _location; }
};
} // namespace spark::error