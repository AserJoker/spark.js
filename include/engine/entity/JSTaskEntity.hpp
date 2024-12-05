#pragma once
#include "engine/entity/JSEntity.hpp"
namespace spark::engine {
class JSTaskEntity : public JSEntity {
private:
  JSEntity *_value;
  size_t _address;

public:
  JSTaskEntity(JSEntity *value, size_t address);
  JSEntity *getValue();
  size_t getAddress();
};
}; // namespace spark::engine