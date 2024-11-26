#pragma once

#include <fmt/core.h>
#include <cstdint>

namespace spark::common {
class Object {
private:
  uint32_t _ref;

public:
  inline const uint32_t &addRef() { return ++_ref; }

  inline const uint32_t &subRef() { return --_ref; }

  inline const uint32_t &ref() const { return _ref; }

  Object() : _ref(0){};

  virtual ~Object() = default;
};
} // namespace spark::common