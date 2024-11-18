#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <math.h>
#include <optional>
#include <string>
#include <vector>

namespace spark::common {

template <class T = uint8_t> class BigInt {
private:
  bool _negative;
  std::vector<T> _data;

public:
  BigInt() : _negative(false), _data({0}) {}

  BigInt(const BigInt &another)
      : _data(another._data), _negative(another._negative) {}

  BigInt(BigInt &&another)
      : _data(another._data), _negative(another._negative) {}

  BigInt(int64_t number) : _negative(number < 0) {
    static uint64_t max = (uint64_t)((T)-1) + 1;
    if (number < 0) {
      number = -number;
    }
    while (number > 0) {
      _data.push_back(number % max);
      number >>= (sizeof(T) * 8);
    }
    if (_data.empty()) {
      _data.push_back(0);
    }
  }

  BigInt(const std::wstring &source) : _negative(false), _data({0}) {
    auto chr = source.c_str();
    if (*chr == L'+') {
      chr++;
    } else if (*chr == L'-') {
      _negative = true;
      chr++;
    }
    while (*chr) {
      *this *= 10;
      *this += (uint64_t)(*chr - '0');
      chr++;
    }
    while (*_data.rbegin() == 0) {
      _data.pop_back();
    }
    if (_data.empty()) {
      _data.push_back(0);
    }
  }

  std::wstring toString() const {
    if (isInfinity()) {
      if (_negative) {
        return L"-Infinity";
      } else {
        return L"Infinity";
      }
    }
    std::wstring result;
    BigInt<> tmp = *this;
    while (tmp > 0) {
      result += (tmp % 10)._data[0] + '0';
      tmp /= 10;
    }
    result = std::wstring(result.rbegin(), result.rend());
    if (_negative) {
      result = L"-" + result;
    }
    return result;
  }

  std::optional<int64_t> toInt64() const {
    if (_data.size() == 1) {
      return _data[0];
    }
    return std::nullopt;
  }

  static BigInt Infinity() {
    BigInt result;
    result._data.clear();
    return result;
  }

  bool isInfinity() const { return _data.empty(); }

  BigInt abs() const {
    if (isInfinity()) {
      return Infinity();
    }
    BigInt result = *this;
    result._negative = true;
    return result;
  }

  BigInt uadd(const BigInt &another) const {
    if (isInfinity() || another.isInfinity()) {
      return BigInt<>::Infinity();
    }
    static uint64_t max = (uint64_t)((T)-1) + 1;
    size_t index = 0;
    uint64_t next = 0;
    BigInt result;
    while (next != 0 || _data.size() > index || another._data.size() > index) {
      uint64_t val = next;
      if (index < _data.size()) {
        val += _data[index];
      }
      if (index < another._data.size()) {
        val += another._data[index];
      }
      if (index >= result._data.size()) {
        result._data.push_back(0);
      }
      result._data[index] = val % max;
      next = val / max;
      index++;
    }
    return result;
  }

  BigInt usub(const BigInt &another) const {
    if (isInfinity() && another.isInfinity()) {
      return BigInt<>((uint64_t)0);
    }
    if (isInfinity() && !another.isInfinity()) {
      return Infinity();
    }
    if (!isInfinity() && another.isInfinity()) {
      return -Infinity();
    }
    BigInt left = abs();
    BigInt right = another.abs();
    BigInt *a = &left;
    BigInt *b = &right;
    bool neg = false;
    if (left > right) {
      a = &right;
      b = &left;
      neg = true;
    }
    int64_t next = 0;
    BigInt result;
    result._data.clear();
    for (size_t index = 0; index < a->_data.size(); index++) {
      uint64_t pa = a->_data[index];
      pa += next;
      if (index < b->_data.size()) {
        uint64_t pb = b->_data[index];
        if (pa < pb) {
          pa += 1 << sizeof(T) * 8;
          next = -1;
        }
        result._data.push_back(pa - pb);
      } else {
        result._data.push_back(pa);
      }
    }
    while (result._data.size() > 1 && *result._data.rbegin() == 0) {
      result._data.pop_back();
    }
    result._negative = neg;
    return result;
  }

  BigInt &operator=(const BigInt &another) {
    if (this == &another) {
      return *this;
    }
    _negative = another._negative;
    _data = another._data;
    return *this;
  }

  BigInt operator+(const BigInt &another) const {
    if (_negative == another._negative) {
      BigInt result = uadd(another);
      result._negative = _negative;
      return result;
    } else {
      return *this - (-another);
    }
  }

  BigInt operator*(const BigInt &another) const {
    if (isInfinity() || another.isInfinity()) {
      auto inf = Infinity();
      inf._negative = _negative != another._negative;
      return inf;
    }
    static uint64_t max = (uint64_t)((T)-1) + 1;
    BigInt result;
    uint64_t rindex = 0;
    for (auto &part : another._data) {
      uint64_t next = 0;
      for (size_t index = 0; index < _data.size(); index++) {
        auto &src = _data[index];
        if (index + rindex >= result._data.size()) {
          result._data.push_back(0);
        }
        auto val = part * src + next;
        result._data[index + rindex] += val % max;
        next = val / max;
      }
      if (next > 0) {
        result._data.push_back(next);
      }
      rindex++;
    }
    while (result._data.size() > 1 && *result._data.rbegin() == 0) {
      result._data.pop_back();
    }
    result._negative = _negative != another._negative;
    return result;
  }

  BigInt operator-(const BigInt &another) const {
    if (_negative == another._negative) {
      BigInt result = usub(another);
      if (_negative) {
        result._negative = !result._negative;
      }
      return result;
    } else {
      return *this + (-another);
    }
  }

  BigInt operator/(const BigInt &another) const {
    if (another == 0) {
      BigInt infinity = Infinity();
      infinity._negative = _negative;
      return infinity;
    }
    if (another > *this) {
      return 0;
    }
    if (another == *this) {
      return 1;
    }
    if (isInfinity()) {
      BigInt infinity = Infinity();
      infinity._negative = _negative;
      return infinity;
    }
    size_t len = _data.size() - another._data.size();
    BigInt next;
    BigInt result;
    std::vector<T> rdata;
    for (auto index = 0; index <= len; index++) {
      BigInt tmp;
      tmp._data.clear();
      std::vector<T> data;
      for (auto i = 0; i < another._data.size(); i++) {
        data.push_back(_data[_data.size() - 1 - index - i]);
      }
      for (auto it = data.rbegin(); it != data.rend(); it++) {
        tmp._data.push_back(*it);
      }
      tmp += next * pow(2, sizeof(T) * 8);
      T v = 0;
      while (tmp >= another) {
        tmp -= another;
        v++;
      }
      next = tmp;
      rdata.push_back(v);
    }
    result._data.clear();
    while (rdata.size() > 1 && *rdata.begin() == 0) {
      rdata.erase(rdata.begin());
    }
    for (auto it = rdata.rbegin(); it != rdata.rend(); it++) {
      result._data.push_back(*it);
    }
    return result;
  }

  BigInt operator%(const BigInt &another) const {
    if (another == 0) {
      BigInt infinity = Infinity();
      infinity._negative = _negative;
      return infinity;
    }
    if (another > *this) {
      return *this;
    }
    if (another == *this) {
      return 0;
    }
    if (isInfinity()) {
      BigInt infinity = Infinity();
      infinity._negative = _negative;
      return infinity;
    }
    size_t len = _data.size() - another._data.size();
    BigInt next;
    for (auto index = 0; index <= len; index++) {
      BigInt tmp;
      tmp._data.clear();
      std::vector<T> data;
      for (auto i = 0; i < another._data.size(); i++) {
        data.push_back(_data[_data.size() - 1 - index - i]);
      }
      for (auto it = data.rbegin(); it != data.rend(); it++) {
        tmp._data.push_back(*it);
      }
      tmp += next * pow(2, sizeof(T) * 8);
      while (tmp >= another) {
        tmp -= another;
      }
      next = tmp;
    }
    next._negative = _negative;
    return next;
  }

  BigInt operator+() const { return *this; }

  BigInt operator-() const {
    BigInt result = *this;
    result._negative = !result._negative;
    return result;
  }

  BigInt operator~() const {
    BigInt result;
    result._data.clear();
    for (auto &part : _data) {
      result._data.push_back(~part);
    }
    if (*result._data.rbegin() == 0) {
      result._data.push_back(1);
    }
    result._negative = !_negative;
    return result;
  }

  bool operator>(const BigInt &another) const {

    if (!_negative && another._negative) {
      return true;
    } else if (_negative && !another._negative) {
      return false;
    }
    if (isInfinity() && !another.isInfinity()) {
      return !_negative;
    }
    if (isInfinity() && another.isInfinity()) {
      return false;
    }
    if (!isInfinity() && another.isInfinity()) {
      return _negative;
    }
    if (_data.size() > another._data.size()) {
      return !_negative;
    } else if (_data.size() < another._data.size()) {
      return _negative;
    }
    for (size_t index = 0; index < _data.size(); index++) {
      if (_data[index] > another._data[index]) {
        return !_negative;
      } else if (_data[index] < another._data[index]) {
        return _negative;
      }
    }
    return false;
  }

  bool operator<(const BigInt &another) const {
    if (!_negative && another._negative) {
      return false;
    } else if (_negative && !another._negative) {
      return true;
    }
    if (isInfinity() && !another.isInfinity()) {
      return _negative;
    }
    if (isInfinity() && another.isInfinity()) {
      return false;
    }
    if (!isInfinity() && another.isInfinity()) {
      return !_negative;
    }
    if (_data.size() > another._data.size()) {
      return _negative;
    } else if (another._data.size() > _data.size()) {
      return !_negative;
    }
    for (size_t index = 0; index < _data.size(); index++) {
      if (_data[index] > another._data[index]) {
        return _negative;
      } else if (_data[index] < another._data[index]) {
        return !_negative;
      }
    }
    return false;
  }

  bool operator>=(const BigInt &another) const {
    if (!_negative && another._negative) {
      return true;
    } else if (_negative && !another._negative) {
      return false;
    }
    if (isInfinity() && !another.isInfinity()) {
      return !_negative;
    }
    if (isInfinity() && another.isInfinity()) {
      return true;
    }
    if (!isInfinity() && another.isInfinity()) {
      return _negative;
    }
    if (_data.size() > another._data.size()) {
      return !_negative;
    } else if (_data.size() < another._data.size()) {
      return _negative;
    }
    for (size_t index = 0; index < _data.size(); index++) {
      if (_data[index] > another._data[index]) {
        return !_negative;
      } else if (_data[index] < another._data[index]) {
        return _negative;
      }
    }
    return true;
  }

  bool operator<=(const BigInt &another) const {
    if (!_negative && another._negative) {
      return false;
    } else if (_negative && !another._negative) {
      return true;
    }
    if (isInfinity() && !another.isInfinity()) {
      return _negative;
    }
    if (isInfinity() && another.isInfinity()) {
      return false;
    }
    if (!isInfinity() && another.isInfinity()) {
      return !_negative;
    }
    if (_data.size() > another._data.size()) {
      return _negative;
    } else if (_data.size() < another._data.size()) {
      return !_negative;
    }
    for (size_t index = 0; index < _data.size(); index++) {
      if (_data[index] > another._data[index]) {
        return _negative;
      } else if (_data[index] < another._data[index]) {
        return !_negative;
      }
    }
    return true;
  }

  bool operator==(const BigInt &another) const {
    if (_negative != another._negative) {
      return false;
    }
    if (_data.size() != another._data.size()) {
      return false;
    }
    for (size_t index = 0; index < _data.size(); index++) {
      if (_data[index] != another._data[index]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const BigInt &another) const {
    if (_negative != another._negative) {
      return true;
    }
    if (_data.size() != another._data.size()) {
      return true;
    }
    for (size_t index = 0; index < _data.size(); index++) {
      if (_data[index] != another._data[index]) {
        return true;
      }
    }
    return false;
  }

  BigInt &operator*=(const BigInt &another) { return *this = *this * another; }

  BigInt &operator+=(const BigInt &another) { return *this = *this + another; }

  BigInt &operator-=(const BigInt &another) { return *this = *this - another; }

  BigInt &operator/=(const BigInt &another) { return *this = *this / another; }

  BigInt &operator%=(const BigInt &another) { return *this = *this % another; }
};
} // namespace spark::common