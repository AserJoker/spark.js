#pragma once
#include "engine/entity/JSObjectEntity.hpp"
#include <vector>
namespace spark::engine {
class JSArrayEntity : public JSObjectEntity {
private:
  std::vector<JSStore *> _items;

public:
  JSArrayEntity(JSStore *prototype);

  const std::vector<JSStore *> &getItems() const;

  std::vector<JSStore *> &getItems();
};
}; // namespace spark::engine