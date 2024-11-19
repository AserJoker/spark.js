#pragma once
#include "engine/entity/JSEntity.hpp"
#include <vector>
namespace spark::engine {
class JSArrayEntity : public JSEntity {
private:
  std::vector<JSEntity *> _items;

public:
  JSArrayEntity();

  const std::vector<JSEntity *> &getItems() const;

  std::vector<JSEntity *> &getItems();
};
}; // namespace spark::engine