#pragma once
#include "JSNode.hpp"
#include "common/Object.hpp"
#include <string>
#include <unordered_map>
#include <vector>
namespace spark::compiler {
struct JSModule : public common::Object {
  std::wstring filename;
  std::unordered_map<uint32_t, JSSourceLocation::Position> sourceMap;
  std::vector<std::wstring> constants;
  std::vector<std::uint8_t> codes;
};
} // namespace spark::compiler