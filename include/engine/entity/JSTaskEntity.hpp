#pragma once
#include "engine/runtime/JSStore.hpp"
namespace spark::engine {
class JSTaskEntity : public JSEntity {
private:
  JSStore *_value;
  size_t _address;

public:
  JSTaskEntity(JSStore *value, size_t address);
  JSStore *getValue();
  size_t getAddress();
};
}; // namespace spark::engine