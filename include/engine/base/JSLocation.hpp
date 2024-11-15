#pragma once
#include <cstdint>
#include <string>
namespace spark::engine {
struct JSLocation {
  uint32_t filename;
  uint32_t line;
  uint32_t column;
  std::wstring funcname;
};
} // namespace spark::engine