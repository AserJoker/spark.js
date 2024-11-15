#pragma once

#include <fmt/core.h>

namespace spark::common {
class Object {
private:
  std::uint32_t _ref;

public:
  inline const std::uint32_t &addRef() { return ++_ref; }

  inline const std::uint32_t &subRef() { return --_ref; }

  inline const std::uint32_t &ref() const { return _ref; }

  Object() : _ref(0){};

  virtual ~Object() = default;
};
} // namespace spark::common