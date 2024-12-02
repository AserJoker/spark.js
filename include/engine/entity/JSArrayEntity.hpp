#pragma once
#include "engine/entity/JSEntity.hpp"
#include "engine/entity/JSObjectEntity.hpp"
#include <vector>
namespace spark::engine {
class JSArrayEntity : public JSObjectEntity {
private:
  std::vector<JSEntity *> _items;

public:
  JSArrayEntity(JSEntity *prototype);

  const std::vector<JSEntity *> &getItems() const;

  std::vector<JSEntity *> &getItems();
};
}; // namespace spark::engine