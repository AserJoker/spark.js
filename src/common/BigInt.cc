#include "common/BigInt.hpp"
#include <iostream>
#include <string>
using namespace spark;
using namespace spark::common;
BigInt::BigInt() : _sign(true) {}
BigInt::BigInt(const std::wstring &src) : _sign(true) {
  uint8_t data[4];
  uint32_t &val = *(uint32_t *)&data[0];
  for (auto it = src.rbegin(); it != src.rend(); it++) {
    std::wcout << *it << std::endl;
  }
}
BigInt::BigInt(const wchar_t *src) : BigInt(std::wstring(src)) {}