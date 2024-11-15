#pragma once
#include <string>
#include <vector>
namespace spark::common {
class BigInt {
private:
  bool _sign;
  std::vector<uint32_t> _data;

public:
  BigInt();
  BigInt(const std::wstring &source);
  BigInt(const wchar_t *source);
};
}; // namespace spark::common